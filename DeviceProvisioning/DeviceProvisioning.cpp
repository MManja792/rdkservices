/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include <fstream>

#include "DeviceProvisioning.h"
#include "utils.h"
#include "socprov_datatype.h"
#include <stdlib.h>
#include <exception>
#include <functional>
#include <openssl/md5.h>
#include <random>
#include <stdexcept>

#ifndef TRACE_SECAPI
#define TRACE_SECAPI 0
#endif

#ifdef SEC_MANAGER_TRACING_ENABLED
#define TRACE_SECAPI 1
#endif

// Convenience methods
#define DP_METHOD_GET_API_VERSION_NUMBER "getApiVersionNumber"
#define DP_METHOD_PROVISION_CHECK "provisionCheck"
#define DP_METHOD_REPROVISION "reprovision"
#define DP_METHOD_NOTIFY_PROVISIONING "notifyProvisioning"
#define DP_METHOD_GET_NETFLIX_ESN "getNetflixESN"
#define DP_METHOD_GET_SOC_ID "getSOCId" // device id
#define DP_METHOD_NOTIFY_RUNTIME_INFORMATION "notifyRuntimeInformation"

// RFU methods
#define DP_METHOD_CHECK_FOR_UPDATES "checkForUpdates"
#define DP_METHOD_SET_AUTOPROVISION_MODE "setAutoProvisionMode"
#define DP_METHOD_GET_AUTOPROVISION_MODE "getAutoProvisionMode"

#define DP_EVT_DID_FINISH_PROVISION_CHECK "didFinishProvisionCheck"
#define DP_EVT_DID_FINISH_REPROVISION "didFinishReprovision"

#define PARAM_SUCCESS "success"
#define PARAM_ERROR "error"

#define PROVISION_TIME_WAIT       25

#define STATUS_SUCCESS            0
#define STATUS_ERROR_OS          -1
#define STATUS_ERROR_TIMEOUT     -2
#define STATUS_ERROR_NOFILE      -3
#define STATUS_ERROR_PARAM       -4

#define BACKOFF_INFO_UNAVAILABLE -1
#define BACKOFF_INFO_FROM_RTI     0
#define BACKOFF_INFO_FROM_FILE    1

#define CURL_CMD "curl -H 'Content-Type: application/x-www-form-urlencoded' "
#define CURL_AUTH "-H 'Authorization: Bearer $TOKEN' -X POST "
#define AUTHSERVICE_CLEAR_AUTH_TOKEN "-d '{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"org.rdk.AuthService.1.clearAuthToken\"}' "
#define AUTHSERVICE_CLEAR_SESSION_TOKEN "-d '{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"org.rdk.AuthService.1.clearSessionToken\"}' "
#define AUTHSERVICE_CLEAR_SERVICE_ACCESS_TOKEN "-d '{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"org.rdk.AuthService.1.clearServiceAccessToken\"}' "
#define SERVICE_API_URL "http://127.0.0.1:9998/jsonrpc"

#define PROVISION_PROPERTIES_FILE "/etc/provision.properties"
#define NETFLIX_ESN_FILENAME "0671000006710001.bin"

#define MAX_OCDM_TIMER_WAIT_ATTEMPTS 10
#define THUNDER_TIMEOUT_IN_MILLISECONDS 2500
#define OCDM_CALLSIGN "OCDM"
#define OCDM_WAIT_TIME_IN_MILLISECONDS 5500
#define POWERMODE_CHANGED_DELAY_SEC 60

#define TIMER_INIT_DELAY_IN_MILLISECONDS 5000
#define TIMER_DEFAULT_CHECK_BACK_SECONDS 60
#define LAUNCH_SOCPROV_CMD "/lib/rdk/launchSocProv.sh #APP_NAME --refresh >>#LOG_NAME 2>&1"

#define RTI "rti" // runtime information prefix
#define DP "dp"   // DeviceProvisionin prefix

//RTI labels
#define BOOTUP_LBL "bootupMode"
#define CLOCKSET_LBL "clockSetReceived"
#define DEVICE_CONNECTED_LBL "deviceIsConnected"
#define FETCH_CALLED_LBL "fetchCredentialsCalled"
#define PROVISION_CALLED_LBL "provisionCredentialsCalled"
#define REFRESH_TIME_LBL "refreshTime"
#define TIME_WAIT_LBL "timeWait"
#define PREFETCH_FAIL_EPOCH_LBL "prefetchFailEpoch"
#define PREFETCH_FAIL_COUNT_LBL "prefetchFailCount"
#define SESSION_START_EPOCH_LBL "sessionStartEpoch"
#define SESSION_DEADLINE_EPOCH_LBL "sessionDeadlineEpoch"
#define BACKOFF_INTERVAL_MAX_LBL "backoffIntervalMax"
#define BACKOFF_EXPIRATION_LBL "backoffExpiration"
#define CTL_RESULT_LBL "controllerResult"
#define FETCH_RESULT_LBL "fetchResult"
#define PROVISION_RESULT_LBL "provisionResult"
#define HEALTHCHECK_RESULT_LBL "healthCheckResult"
#define PROVISION_OPTIONS_LBL "provisionOptions"
#define PROVISION_TYPE_LBL "provisionType"
#define FINAL_LBL "final"
// DP labels
#define APP_SHOULD_EXIT_LBL "shouldExit"
#define APP_SHOULD_EXIT_MESSAGE_LBL "shouldExitReason"
#define LASTCALL_LBL "lastCallEpoch" //last call epoch time for the app

#define LOGI_LOCAL(fmt, ...) do { fprintf(stderr, "[%d] INFO [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGD_LOCAL(fmt, ...) do { fprintf(stderr, "[%d] DEBUG [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGW_LOCAL(fmt, ...) do { fprintf(stderr, "[%d] WARN [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGE_LOCAL(fmt, ...) do { fprintf(stderr, "[%d] ERROR [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 1
#define API_VERSION_NUMBER_PATCH 0

static WPEFramework::Core::CriticalSection gTimerLock;
string WPEFramework::Plugin::DeviceProvisioning::RT_REFRESH_EPOCH_FILE_NAME = "rtrefresh-epoch";
string WPEFramework::Plugin::DeviceProvisioning::RT_BACKOFF_PERSISTENT_FILE_NAME = "rtbackoff-info";
IARM_Bus_PWRMgr_PowerState_t WPEFramework::Plugin::DeviceProvisioning::_powerState = IARM_BUS_PWRMGR_POWERSTATE_OFF;
uint64_t WPEFramework::Plugin::DeviceProvisioning::_wakeupTime = 0;
/// Command templates;
// $ indicates a session-wide replacement typically performed in Constructor
// # indicates a replacement that is supposed to be done each time before the invocation

std::string provisionTypeToAppName(const std::string& provisionType)
{
    std::string result;
    const std::map<std::string, std::string> appName = {
              { "HARDWARE", "socprovisioning"}
            , { "CRYPTANIUM", "socprovisioning-crypto"}
    };
    try
    {
        result = appName.at(provisionType);
    }
    catch (const std::out_of_range& oor)
    {
        UNUSED(oor);
        result = "UNKNOWN";
    }
    return result;
}

std::string AppNameToLogName(const std::string& appName)
{
    std::string result;
    const std::map<std::string, std::string> logName = {
              { "socprovisioning", "/opt/logs/socprov.log"}
            , { "socprovisioning-crypto", "/opt/logs/socprov-crypto.log"}
    };
    try
    {
        result = logName.at(appName);
    }
    catch (const std::out_of_range& oor)
    {
        UNUSED(oor);
        result = "/opt/logs/socprov.log";
    }
    return result;
}

string WPEFramework::Plugin::DeviceProvisioning::GET_DRM_DIR_CMD = R"shell(
        source $FILENAME;
        echo ${#PROVISION_TYPE[0]};
    )shell";

// This a compatibility filter that returns the supported provision types regardless of the file format (v1 or v2).
string WPEFramework::Plugin::DeviceProvisioning::PROVISION_PROPERTIES_FILTER = R"shell(
    export PPFL=$FILENAME;
    export EXPR1="=\(.+\)";
    export EXPR2="s/[=(|)]//g";
    export EXPR3="s/\s/\n/g";
    cat $PPFL | grep = | wc -l | awk '{if ($1=="0") system("cat $PPFL"); else system("cat $PPFL | grep -o -E $EXPR1 | sed $EXPR2 | sed $EXPR3")}';
    )shell";

// This command returns the supported provision types for RT provtocol version 2
string WPEFramework::Plugin::DeviceProvisioning::GET_PROVISION_TYPES_CMD = R"shell(
        source $FILENAME;
        echo ${TYPES[*]};
    )shell";
///
// End command templates

string WPEFramework::Plugin::DeviceProvisioning::GET_NETFLIX_ESN_CMD = "cat #PRIMARY_DRM_DIR/#NETFLIX_ESN_FILENAME; echo;";

namespace Sec { // Similar to secmanager in SecApiUtils
    class Utils
    {
    public:
        static std::string bytes_to_hex(std::vector<SEC_BYTE> bytes)
        {
            char symbols[] = "0123456789abcdef";
            std::string out = "" ;
            for(std::size_t i = 0; i < bytes.size(); ++i)
            {
                unsigned char c = bytes.at(i);
                int r = c % 16 ;
                int nn = c / 16 ;
                out += symbols[nn];
                out += symbols[r];
            }
            return out;
        }
    };

    class SecApiException : public std::exception {
    public:
        SecApiException() : _txt("SecApiException") {}
        SecApiException(const char* txt) { _txt = std::string("SecApiException: ") + std::string(txt); }

        virtual const char* what() const noexcept {
            return _txt.c_str();
        }
    private:
        std::string _txt;
    };

    class SecApi {
    public:
        static Sec_ProcessorHandle* createProcessor(const char* globalDir, const char* appDir);
        static std::string secApiVersion(Sec_ProcessorHandle* proc);
        static std::vector<SEC_BYTE> deviceId(Sec_ProcessorHandle* proc);
    };

    Sec_ProcessorHandle* SecApi::createProcessor(const char* globalDir, const char* appDir) {
        SEC_TRACE(TRACE_SECAPI, "createProcessor globalDir: %s, appDir: %s", globalDir, appDir);

        Sec_ProcessorHandle* proc = NULL;

        if (SEC_RESULT_SUCCESS != SecProcessor_GetInstance_Directories(&proc, globalDir, appDir)) {
            SEC_LOG_ERROR("SecProcessor_GetInstance failed");
            throw SecApiException();
        }

        return proc;
    }

    std::string SecApi::secApiVersion(Sec_ProcessorHandle* proc) {
        SEC_TRACE(TRACE_SECAPI, "secApiVersion");

        Sec_ProcessorInfo info;

        if (SEC_RESULT_SUCCESS != SecProcessor_GetInfo(proc, &info)) {
            SEC_LOG_ERROR("SecProcessor_GetInfo failed");
            throw SecApiException();
        }

        return std::string((const char*) info.version);
    }

    std::vector<SEC_BYTE> SecApi::deviceId(Sec_ProcessorHandle* proc) {
        SEC_TRACE(TRACE_SECAPI, "deviceId");

        std::vector<SEC_BYTE> deviceId(SEC_DEVICEID_LEN);
        if (SEC_RESULT_SUCCESS != SecProcessor_GetDeviceId(proc, &deviceId[0])) {
            SEC_LOG_ERROR("SecProcessor_GetDeviceId failed");
            throw SecApiException();
        }

        return deviceId;
    }
}

namespace Utils
{
    uint64_t getCurrentEpoch()
    {
        uint64_t res = 0;
        auto now = std::chrono::system_clock::now();
        res = std::chrono::time_point_cast<std::chrono::seconds>(now).time_since_epoch().count();
        return res;
    }

    int64_t getRandomValue( int64_t rangeLo, int64_t rangeHi)
    {
        std::random_device dev;
        std::default_random_engine engine(dev());
        std::uniform_int_distribution<int64_t> randomInt( rangeLo, rangeHi );
        int64_t deviation = randomInt(engine );
        return deviation;
    }

    // Supported range: 0..23. Delay with fibonacci32[23] is ~21 hour
    uint64_t fibo24(const uint64_t n)
    {
        // The first two numbers 0, 1 are dropped to make it less aggressive initially
        const std::vector<uint> fibonacci24 = {1, 2, 3, 5, 8, 13, 21, 34,
                                               55, 89, 144, 233, 377, 610, 987, 1597,
                                               2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025};
        uint64_t lo, hi;
        lo = 0; hi = static_cast<uint64_t>(fibonacci24.size() - 1);
        uint64_t idx = clamp(n, lo, hi);
        return fibonacci24[idx];
    }

    // Supported range: 0..31. Delay with fibonacci32[31] is ~41 day
    uint64_t fibo32(const uint64_t n)
    {
        // The first two numbers 0, 1 are dropped to make it less aggressive initially
        const std::vector<uint> fibonacci32 = {1, 2, 3, 5, 8, 13, 21, 34,
                                               55, 89, 144, 233, 377, 610, 987, 1597,
                                               2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025,
                                               121393, 196418, 317811, 514229, 832040, 1346269, 2178309, 3524578};
        uint64_t lo, hi;
        lo = 0; hi = static_cast<uint64_t>(fibonacci32.size() - 1);
        uint64_t idx = clamp(n, lo, hi);
        return fibonacci32[idx];
    }

    //#define USE_FIBO24
    uint64_t fibo(const uint64_t n)
    {
    #if (defined(USE_FIBO24))
        return fibo24(n);
    #else
        return fibo32(n);
    #endif
    }

    // Calculates the backoff expiration time from the failure time and fail count.
    // The resulting value is a point in time with failCount converted to wait time in seconds
    // based on the Fibonacci sequence, which is also randomized to +/- 25% and clamped by backoffIntervalMax
    uint64_t calcBackoffExpiration(const uint64_t prefetchFailCount, const uint64_t prefetchFailEpoch, const uint64_t backoffIntervalMax = fibo(31))
    {
        std::random_device dev;
        std::default_random_engine engine(dev());
        auto backoffInterval = fibo(prefetchFailCount);
        if (backoffInterval >= 32) { // not deviating for small values
            auto range =  (int64_t)(backoffInterval / 4);
            std::uniform_int_distribution<int64_t> randomInt( -range, range );
            int64_t deviation = randomInt( engine );
            backoffInterval += deviation;
        }
        uint64_t expiration =  prefetchFailEpoch + clamp(backoffInterval, (uint64_t)0, backoffIntervalMax);
        return expiration;
    }

    // if the backoff happens to be too far in the past, we bring it closer so that device
    // would not wait for too long before the next attempt (it would for 144 seconds)
    bool adjustBackoff(uint64_t& prefetchFailCount, uint64_t& prefetchFailEpoch)
    {
        bool res = false;
        if (prefetchFailCount > 0)
        {
            const uint64_t oneDay = 86400; // sec
            const uint64_t defFailCount = 10; // 144 seconds wait
            uint64_t now = Utils::getCurrentEpoch();
            uint64_t backoff = Utils::calcBackoffExpiration(prefetchFailCount, prefetchFailEpoch, fibo(31));
            if ((backoff < now) && ((now - backoff) > oneDay))
            {
                prefetchFailEpoch = now - fibo(defFailCount);
                prefetchFailCount = defFailCount;
                res = true;
            }
        }
        return res;
    }
    // This function performs an md5 check
    // param fileName must be fully qualified
    // filename.md5 is expected to be available in the same directory, for example 0651000006510001.bin has 0651000006510001.bin.md5
    // if the md5 of the filename is the same as the contents of the filename.md5, the result is true, otherwise it is false
    bool coherent(const std::string& fileName)
    {
        bool res = false;
        std::string fileNameMd5 = fileName + ".md5";
        FILE *fh;
        FILE *fm;
        long filesize;
        unsigned char* buf;
        unsigned char* md5_result = nullptr;
        char* md5_result_str = nullptr;
        char* md5_stored_str = nullptr;

        fh = fopen(fileName.c_str(), "r");
        fm = fopen(fileNameMd5.c_str(), "r");
        if (fh && fm)
        {
            fseek(fh, 0L, SEEK_END);
            filesize = ftell(fh);
            fseek(fh, 0L, SEEK_SET);
            buf = (unsigned char*)malloc(filesize);
            fread(buf, filesize, 1, fh);
            md5_result = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
            md5_result_str = (char*)malloc(2 * MD5_DIGEST_LENGTH + 1);
            MD5(buf, filesize, md5_result);
            for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            {
                sprintf(md5_result_str + 2*i, "%02x",  md5_result[i]);
            }
            md5_stored_str = (char*)malloc(2 * MD5_DIGEST_LENGTH + 1);
            fgets(md5_stored_str, 2 * MD5_DIGEST_LENGTH + 1, fm);
            //TRACE_GLOBAL(Trace::Information, (_T("calculated md5: %s"), md5_result_str));
            //TRACE_GLOBAL(Trace::Information, (_T("stored     md5: %s"), md5_stored_str));
            if (strcmp(md5_result_str, md5_stored_str) == 0)
            {
                res = true;
            } else {
                res = false;
            }
            free(md5_result);
            free(md5_result_str);
            free(md5_stored_str);
            free(buf);
        } else {
            std::stringstream errorMsg;
            errorMsg << "Could not find ";
            if (!fh)
            {
                errorMsg << fileName;
            }
            if (!fm)
            {
                errorMsg << "; " << fileNameMd5;
            }
            LOGE_LOCAL("%s", errorMsg.str().c_str());
            //TRACE_GLOBAL(Trace::Fatal, (_T("%s"), errorMsg.str().c_str()));
            res = false;
        }
        if (fm){
            fclose(fm);
        }
        if (fh){
            fclose(fh);
        }
        return res;
    }
} // Utils

namespace DeviceInfo
{
    uint64_t getRefreshEpochFromFile(const std::string& drmDir) // returns current epoch if the file is missing
    {
        uint64_t res = Utils::getCurrentEpoch();
        std::string fileName = drmDir + "/" + WPEFramework::Plugin::DeviceProvisioning::RT_REFRESH_EPOCH_FILE_NAME;
        if (Utils::coherent(fileName))
        {
            std::ifstream input(fileName);
            std::string refreshTimeStr;
            char* end;
            if (std::getline( input, refreshTimeStr))
            {
                res = strtoull(refreshTimeStr.c_str(), &end, 10 );
            } else {
                LOGE_LOCAL("Unable to read from file %s ", fileName.c_str());
            }
        } else {
            LOGE_LOCAL("File %s appears to be missing or corrupted.", fileName.c_str());
        }
        return res;
    }

    bool putRefreshEpochToFile(const uint64_t refreshEpoch, const std::string& drmDir)
    {
        bool res = false;
        std::string fileName = drmDir + "/" + WPEFramework::Plugin::DeviceProvisioning::RT_REFRESH_EPOCH_FILE_NAME;
        std::string fileNameMd5 = fileName + ".md5";
        FILE *fh;
        FILE *fm;
        char buf[32] = "";
        unsigned char* md5_result = nullptr;
        char* md5_result_str = nullptr;
        LOGI_LOCAL("putRefreshEpochToFile called with params %llu and %s", refreshEpoch, fileName.c_str());
        fh = fopen(fileName.c_str(), "w+");
        fm = fopen(fileNameMd5.c_str(), "w+");
        if (fh && fm)
        {
            md5_result = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
            md5_result_str = (char*)malloc(2 * MD5_DIGEST_LENGTH);
            sprintf(buf, "%llu", refreshEpoch);
            MD5((const unsigned char*)buf, strlen(buf), md5_result);
            for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            {
                sprintf(md5_result_str + 2*i, "%02x",  md5_result[i]);
            }
            fputs(buf, fh);
            fputs(md5_result_str, fm);

            free(md5_result);
            free(md5_result_str);
            res = true;
        } else {
            std::stringstream errorMsg;
            errorMsg << "Could not open ";
            if (!fh)
            {
                errorMsg << fileName;
            }
            if (!fm)
            {
                errorMsg << "; " << fileNameMd5;
            }
            errorMsg << " for writing.";
            LOGE_LOCAL("%s", errorMsg.str().c_str());
            res = false;
        }
        if (fm){
            fclose(fm);
        }
        if (fh){
            fclose(fh);
        }
        return res;
    }

    int getBackoffInfoFromFile( const std::string& drmDir, uint64_t& prefetchFailCount, uint64_t& prefetchFailEpoch)
    {
        int res = BACKOFF_INFO_UNAVAILABLE;
        std::string fileName = drmDir + "/" + WPEFramework::Plugin::DeviceProvisioning::RT_BACKOFF_PERSISTENT_FILE_NAME;
        if (Utils::coherent(fileName))
        {
            std::ifstream input(fileName);
            std::string prefetchFailCountStr;
            std::string prefetchFailEpochStr;
            char* end;
            if (std::getline( input, prefetchFailEpochStr))
            {
                prefetchFailEpoch = strtoull(prefetchFailEpochStr.c_str(), &end, 10 );
                res = true;
                if (std::getline( input, prefetchFailCountStr)) // optional
                {
                    prefetchFailCount = strtoull(prefetchFailCountStr.c_str(), &end, 10 );
                }
                res = BACKOFF_INFO_FROM_FILE;
            } else {
                LOGE_LOCAL("Unable to read from file %s ", fileName.c_str());
                //TRACE_GLOBAL(Trace::Fatal, (_T("Unable to read from file %s "), fileName.c_str()));
            }
        } else {
            LOGE_LOCAL("File %s appears to be missing or corrupted.", fileName.c_str());
            //TRACE_GLOBAL(Trace::Fatal, (_T("File %s appears to be missing or corrupted."), fileName.c_str()));
        }
        return res;
    }
} // DeviceInfo


class RegVal // Registry value, a quick replacement for std::variant, see Reg
{
public:
    explicit RegVal(uint64_t val): u64Val(val), typeName(typeid(u64Val).name()) {}
    explicit RegVal(int64_t val) : i64Val(val), typeName(typeid(i64Val).name()) {}
    explicit RegVal(int32_t val) : i32Val(val), typeName(typeid(i32Val).name()) {}
    explicit RegVal(bool val): boolVal(val), typeName(typeid(bool).name()) {}
    explicit RegVal(std::string  val): stdStrVal(std::move(val)), typeName(typeid(std::string).name()) {}
    explicit RegVal(const char*  val): stdStrVal(val), typeName(typeid(std::string).name()) {} // storing c-string in std::string

    uint64_t getU64() const { return  u64Val; }
    int64_t  getI64() const {return  i64Val; }
    int32_t  getI32() const {return  i32Val; }
    bool getBool() const {return boolVal; }
    const std::string& getStdStr() const { return stdStrVal; };
    const char* getCStr() const { return stdStrVal.c_str(); };

    void setU64(uint64_t val) { if (u64Val != val) u64Val = val; };
    void setI64(int64_t val) { if (i64Val != val) i64Val = val; };
    void setI32(int32_t val) { if (i32Val != val) i32Val = val; };
    void setBool(bool val) { if (boolVal != val) boolVal = val; };
    void setStdStr(const std::string& val) { stdStrVal = val; }
    void setCStr(const char* val) { stdStrVal = val; }
private:
    uint64_t u64Val = 0;
    int64_t i64Val = 0;
    int32_t i32Val = 0;
    bool boolVal = false;
    std::string stdStrVal;
public:
    const char* typeName; // typeName defines which of the members is holding the value
};

class Reg // Flexible string-value container with erase() and contains() methods
{
public:
    ~Reg() {clear();}
    void setVal(const std::string& label, uint64_t val) { erase(label); m_values.emplace(label, RegVal(val)); }
    void setVal(const std::string& label, int64_t val) { erase(label); m_values.emplace(label, RegVal(val)); }
    void setVal(const std::string& label, int32_t val) { erase(label); m_values.emplace(label, RegVal(val)); }
    void setVal(const std::string& label, bool val) { erase(label); m_values.emplace(label, RegVal(val)); }
    void setVal(const std::string& label, const std::string& val) { erase(label); m_values.emplace(label, RegVal(val)); }
    void setVal(const std::string& label, const char* val) { erase(label); m_values.emplace(label, RegVal(val)); }

    uint64_t getU64Val (const std::string& label) const {return m_values.at(label).getU64();}
    int64_t getI64Val(const std::string& label) const {return m_values.at(label).getI64();}
    int32_t getI32Val(const std::string& label) const {return m_values.at(label).getI32();}
    bool getBoolVal(const std::string& label) const {return m_values.at(label).getBool();}
    const std::string& getStdStrVal (const std::string& label) const {return m_values.at(label).getStdStr();}
    const char* getCStrVal (const std::string& label) const {return m_values.at(label).getCStr();}

    bool contains(const std::string& label) const { auto search = m_values.find(label); return (search != m_values.end()); } // works also for substrings
    void erase(const std::string& label) { auto search = m_values.find(label); if (search != m_values.end()) m_values.erase(search); }

    void eraseByPrefix(const std::string& prefix)
    {
        auto it = m_values.begin();
        while (it != m_values.end())
        {
            const std::string& key = it->first;
            if (key.compare(0, prefix.size(), prefix) == 0)
            {
                it = m_values.erase(it);
            } else {
                ++it;
            }
        }
    }

    static const std::string& path(const std::vector<std::string>& pathParts, const std::string& label)
    {
        static std::string thePath;
        thePath.clear();
        for(std::string pathPart : pathParts)
        {
            thePath += pathPart;
            thePath += "/";
        }
        thePath += label;
        return thePath;
    }

    static const std::vector<std::string>& pathParts(const std::string& path)
    {
        static  std::vector<std::string> parts;
        parts.clear();
        std::istringstream f(path);
        std::string s;
        while (getline(f, s, '/')) {
            parts.push_back(s);
        }
        return parts;
    }

    void incVal(const std::string& label, uint64_t val = 1)
    {
        uint64_t newVal;
        if (contains(label))
        {
            newVal = getU64Val(label) + val;
        } else {
            newVal = val;
        }
        setVal(label, newVal);
    }

    void clear()
    {
        m_values.clear();
    }

    const std::string& toMultilineString(const std::string& prefix)
    {
        static std::string result;
        std::stringstream kvpairs;
        auto it = m_values.begin();
        while (it != m_values.end())
        {
            const auto pair = *it;
            const std::string& key = pair.first;
            if (key.compare(0, prefix.size(), prefix) == 0)
            {
                kvpairs << key << " : ";
                if (typeid(uint64_t).name() == pair.second.typeName)
                {
                    kvpairs << pair.second.getU64() << '\n';
                } else if (typeid(int64_t).name() == pair.second.typeName)
                {
                    kvpairs << pair.second.getI64() << '\n';
                } else if (typeid(int32_t).name() == pair.second.typeName)
                {
                    kvpairs << pair.second.getI32() << '\n';
                } else if (typeid(bool).name() == pair.second.typeName)
                {
                    kvpairs << (pair.second.getBool() ? "true" : "false") << '\n';
                } else if (typeid(std::string).name() == pair.second.typeName)
                {
                    kvpairs << pair.second.getStdStr() << '\n';
                }
            }
            ++it;
        }
        result = kvpairs.str();
        return result;
    }


private:
    std::unordered_map<std::string, RegVal> m_values;
};

namespace {
    template<typename T, typename... Args>
    void RegisterMethod(T* obj, std::initializer_list<int> versions, Args&&... args) {
        for (auto version : versions) {
            obj->GetHandler(version)->template Register<JsonObject, JsonObject>(std::forward<Args>(args)...);
        }
    }
}

namespace WPEFramework
{
    namespace {
        static Plugin::Metadata<Plugin::DeviceProvisioning> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace Plugin
    {
        DeviceProvisioning* DeviceProvisioning::_instance = nullptr;

        DeviceProvisioning::DeviceProvisioning()
        : AbstractPlugin(API_VERSION_NUMBER_MAJOR)
        , _apiVersionNumber(API_VERSION_NUMBER_MAJOR)
        , _service(nullptr)
        , _subSystem(nullptr)
        {
            DeviceProvisioning::_instance = this;
            _ocdmTimer.connect(std::bind(&DeviceProvisioning::onOCDMTimer, this));

            CreateHandler({2});
            CreateHandler({3});

            RegisterMethod(this, {1}, DP_METHOD_GET_API_VERSION_NUMBER, &DeviceProvisioning::getApiVersionNumber, this);
            RegisterMethod(this, {1}, DP_METHOD_PROVISION_CHECK, &DeviceProvisioning::provisionCheckSync, this);
            RegisterMethod(this, {1}, DP_METHOD_REPROVISION, &DeviceProvisioning::reprovisionSync, this);

            RegisterMethod(this, {1,2}, DP_METHOD_GET_NETFLIX_ESN, &DeviceProvisioning::getNetflixEsnSync, this);
            RegisterMethod(this, {1,2}, DP_METHOD_NOTIFY_PROVISIONING, &DeviceProvisioning::notifyProvisioningSync, this);

            RegisterMethod(this, {1,3}, DP_METHOD_GET_SOC_ID, &DeviceProvisioning::getSOCIdSync, this);
            RegisterMethod(this, {1,3}, DP_METHOD_NOTIFY_RUNTIME_INFORMATION, &DeviceProvisioning::notifyRuntimeInformationSync, this);

            // RFU (stubs)
            RegisterMethod(this, {1}, DP_METHOD_CHECK_FOR_UPDATES, &DeviceProvisioning::checkForUpdatesSync, this);
            RegisterMethod(this, {1}, DP_METHOD_SET_AUTOPROVISION_MODE, &DeviceProvisioning::setAutoProvisionMode, this);
            RegisterMethod(this, {1}, DP_METHOD_GET_AUTOPROVISION_MODE, &DeviceProvisioning::getAutoProvisionMode, this);

            /// Some run-time replacements to our command templates for this session
            //
            Utils::String::replace_substr(PROVISION_PROPERTIES_FILTER, "$FILENAME", PROVISION_PROPERTIES_FILE);
            Utils::String::replace_substr(GET_DRM_DIR_CMD, "$FILENAME", PROVISION_PROPERTIES_FILE);
            Utils::String::replace_substr(GET_PROVISION_TYPES_CMD, "$FILENAME", PROVISION_PROPERTIES_FILE);
            //
            /// End replacements
        }

        DeviceProvisioning::~DeviceProvisioning()
        {
        }

        // IProvisioning Methods begin
        //
        void DeviceProvisioning::Register(IProvisioning::INotification* provisioning)
        {
            _eventLock.Lock();
            //Make sure a provisioning is not registered multiple times.
            if (std::find(_observers.begin(), _observers.end(), provisioning) == _observers.end())
            {
                TRACE(Trace::Information, (_T("Registered a client with Provisioning")));
                _observers.push_back(provisioning);
                provisioning->AddRef();

            }
            _eventLock.Unlock();
        }

        void DeviceProvisioning::Unregister(IProvisioning::INotification* provisioning)
        {
            _eventLock.Lock();
            std::list<Exchange::IProvisioning::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), provisioning));
            // Make sure you do not unregister something you did not register !!!
            if (index != _observers.end()) {
                (*index)->Release();
                _observers.erase(index);
            }
            _eventLock.Unlock();
        }
        //
        // IProvisioning Methods begin

        const string DeviceProvisioning::Initialize(PluginHost::IShell* service)
        {
            std::string message;
            std::stringstream errorMsg;

            _reg = new Reg;
            ASSERT(_reg != nullptr);

            if(_ocdmTimer.isActive()) {
                _ocdmTimer.stop();
            }
            _ocdmTimer.start(OCDM_WAIT_TIME_IN_MILLISECONDS);

            ASSERT(service != nullptr);
            ASSERT(_service == nullptr);
            ASSERT(_subSystem == nullptr);

            _subSystem = service->SubSystems();
            _service = service;

            getPrimaryDrmDir(_primaryDrmDir);
            if (getSupportedProvisionTypes().empty())
            {
                message = "Could not determine the supported provision types. Check the provision properties file";
            }

            // Checking the supported types of provisioning for which the device is actually provisioned
            std::vector<std::string> provisionedTypes;
            getProvisionedTypes(provisionedTypes);

            // setting up the SoC Provisioning timers
            static std::vector<std::string>& supportedProvisionTypes = getSupportedProvisionTypes();
            if (!supportedProvisionTypes.empty())
            {
                std::vector<std::string>::const_iterator pt = supportedProvisionTypes.begin();
                std::string drmDir;
                while (pt != supportedProvisionTypes.end())
                {
                    if(getDrmDir(*pt, drmDir))
                    {
                        TpTimer *pTimer = new TpTimer;
                        TRACE(Trace::Fatal, (_T("Created a timer %p for %s (%s)"), pTimer, (*pt).c_str(), drmDir.c_str()));
                        auto onSocProvTimerCb = std::bind(&DeviceProvisioning::onSocProvTimer, this, (*pt), pTimer);
                        pTimer->connect(std::bind(onSocProvTimerCb, this));
                        int64_t range = TIMER_INIT_DELAY_IN_MILLISECONDS / 2;
                        pTimer->start(TIMER_INIT_DELAY_IN_MILLISECONDS + Utils::getRandomValue(-range, range));
                        _socprovTimers.emplace(*pt, pTimer);
                    } else {
                        TRACE(Trace::Fatal, (_T("SoC Provisioning refresh-time invocation could not be configured for %s: unable to get DRM dir for this provisioning type. Please check %s"), (*pt).c_str(), PROVISION_PROPERTIES_FILE));
                    }
                    ++pt;
                }
            } else {
                TRACE(Trace::Fatal, (_T("SoC Provisioning refresh-time invocations could not be configured: device does not have any supported provision types. Please check %s"), PROVISION_PROPERTIES_FILE));
            }

            if (!provisionedTypes.empty())
            {
                fulfillPrecondition(provisionedTypes, "quick check on Initialize");
            } else {
                TRACE(Trace::Fatal, (_T("Device does not have any DRM packages. PROVISIONING precondition could not be fulfilled yet. Waiting for the signal from SocProvisioning")));
            }

            try{
                _sec = std::shared_ptr<Sec_ProcessorHandle>(Sec::SecApi::createProcessor(_primaryDrmDir.c_str(), _primaryDrmDir.c_str()), SecProcessor_Release);
            }
            catch (const Sec::SecApiException& e) {
                errorMsg << "Sec API create processor failed exception caught - " << e.what();
                message = errorMsg.str();
                TRACE(Trace::Fatal, (_T(message.c_str())));
            }
            if (Utils::IARM::init())
            {
                IARM_Bus_PWRMgr_GetPowerState_Param_t param;
                IARM_Result_t res = IARM_Bus_Call(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_API_GetPowerState,
                (void *)&param, sizeof(param));
                if (res == IARM_RESULT_SUCCESS)
                {
                    _powerState = param.curState;
                }
                TRACE(Trace::Information, (_T("DeviceProvisioning: registering IARM_BUS_PWRMGR_EVENT_MODECHANGED current state: %d"), _powerState));
                IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME, IARM_BUS_PWRMGR_EVENT_MODECHANGED, powerModeChange);

                IARM_Bus_RegisterEventHandler(IARM_BUS_SOCPROVISIONING_NAME, IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_REPROVISIONED, _iarmEventHandler);
                IARM_Bus_RegisterEventHandler(IARM_BUS_SOCPROVISIONING_NAME, IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_PROVISIONED, _iarmEventHandler);
            } else {
                TRACE(Trace::Fatal, (_T("Could not initialize IARM_BUS")));
            }
            // On success return empty, to indicate there is no error text.
            return(message);
        }

        void DeviceProvisioning::Deinitialize(PluginHost::IShell* service)
        {
            if(_ocdmTimer.isActive()) {
                _ocdmTimer.stop();
            }

            std::unordered_map<std::string, TpTimer*>::const_iterator it = _socprovTimers.begin();
            while (it != _socprovTimers.end())
            {
                if(it->second->isActive())
                {
                    it->second->stop();
                }
                delete it->second;
                it = _socprovTimers.erase(it);
            }
            _socprovTimers.clear();

            if (_executionThread.joinable())
                _executionThread.join();

            auto trunk = _socprovThreads.begin();
            while (trunk != _socprovThreads.end())
            {
                if(trunk->second->joinable())
                {
                    trunk->second->join();
                    delete trunk->second;
                    trunk->second = nullptr;
                }
                ++trunk;
            }
            _socprovThreads.clear();

            ASSERT(_service == service);

            if (_subSystem != nullptr) {
                _subSystem->Release();
                _subSystem = nullptr;
            }

            _service = nullptr;
             DeviceProvisioning::_instance = nullptr;

             delete _reg;
            _reg = nullptr;
        }

        void DeviceProvisioning::powerModeChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if (strcmp(owner, IARM_BUS_PWRMGR_NAME)  == 0)
            {
                switch (eventId) {
                    case IARM_BUS_PWRMGR_EVENT_MODECHANGED:
                    {
                        IARM_Bus_PWRMgr_EventData_t *param = (IARM_Bus_PWRMgr_EventData_t *)data;
                        TRACE_GLOBAL(Trace::Fatal, (_T("DeviceProvisioning Event IARM_BUS_PWRMGR_EVENT_MODECHANGED: new State: %d"), param->data.state.newState));

                        if (_powerState != param->data.state.newState && (param->data.state.newState != IARM_BUS_PWRMGR_POWERSTATE_ON && param->data.state.newState != IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP))
                        {
                            TRACE_GLOBAL(Trace::Fatal, (_T("Device is being suspended")));
                        }

                        if (_powerState != param->data.state.newState && param->data.state.newState == IARM_BUS_PWRMGR_POWERSTATE_ON)
                        {
                            TRACE_GLOBAL(Trace::Fatal, (_T("Device just woke up")));
                            _wakeupTime = Utils::getCurrentEpoch();
                        }
                        _powerState = param->data.state.newState;
                    }
                        break;
                    default:
                        break;
                }
            }
        }

        void DeviceProvisioning::_iarmEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
             if (DeviceProvisioning::_instance)
                 DeviceProvisioning::_instance->iarmEventHandler(owner, eventId, data, len);
        }

        void DeviceProvisioning::iarmEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
             IARM_BUS_SocProv_EventData_t *param = (IARM_BUS_SocProv_EventData_t *)data;
	     std::vector<string> provisionTypes;
	     provisionTypes.push_back(param->type);

	     std::vector<std::string>::const_iterator pt = provisionTypes.begin();

             while (pt != provisionTypes.end())
             {
                 if (*pt == "CRYPTANIUM")
                 {
                     TRACE(Trace::Warning, (_T("CRYPTANIUM provision type Ignored.")));
                     pt = provisionTypes.erase(pt);
                 } else {
                     ++pt;
                 }
             }

             if (strcmp(owner, IARM_BUS_SOCPROVISIONING_NAME) == 0)
             {
                 switch (eventId) {
                     case IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_PROVISIONED:
                          LOGI_LOCAL("Triggerring IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_PROVISIONED");
                          break;
                     case IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_REPROVISIONED:
                          LOGI_LOCAL("Triggerring IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_REPROVISIONED");
                          break;
                     default:
                          break;
                 }

                 LOGI_LOCAL("notification from SocProvisioning");
                 if (_subSystem != nullptr && !provisionTypes.empty()) {
                     Core::Sink<RPC::StringIterator> information (provisionTypes);
                     _subSystem->Set(PluginHost::ISubSystem::PROVISIONING, &information);

                     // notifying observers
                      _eventLock.Lock();
                      std::list<Exchange::IProvisioning::INotification*>::const_iterator client = _observers.begin();
                      while (client != _observers.end()) {
                            (*client)->Provisioned(&information);
                            client++;
                      }
                      _eventLock.Unlock();
                  }

              }

        }

        /// Internal methods begin
        //
        // Blocking call, meant to be executed in a thread
        bool DeviceProvisioning::isProcessRunning(const std::string& processName)
        {
             DIR* dir;
             struct dirent* ent;
             char* endptr;
             bool res = false;
             char buffer[256] = {0};
             unsigned long lpid;

             if (!(dir = opendir("/proc")))  {
                  TRACE_GLOBAL(Trace::Fatal, (_T("can't open /proc %d"), errno));
                  return res;
             }

             while ((ent = readdir(dir)) != NULL) {
                     /* if endptr is not a null character, the directory is not
                      * entirely numeric, so ignore it */
                 lpid = strtol(ent->d_name, &endptr, 10);
                 if (*endptr != '\0') {
                     continue;
                 }

                 /* try to open the cmdline file */
                 snprintf(buffer, sizeof(buffer), "/proc/%ld/cmdline", lpid);
                 FILE* fp = fopen(buffer, "r");
                 if (fp) {
                    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                        /* check the first token in the file, the processName */
                        char* first = strtok(buffer, " ");
                        if (!strcmp(first, processName.c_str())) {
                            fclose(fp);
                            break;
                        }
                    } else {
                         TRACE_GLOBAL(Trace::Fatal, (_T("isProcessRunning: no outpupt.  Assuming process is NOT running")));
                    }
                    fclose(fp);
                 }

             }

             /*sending 0 signal to kill is used to check if process running*/
             int rc = kill(lpid, 0);
	     if (!rc) {
                 res = true;
             }

             TRACE_GLOBAL(Trace::Information, (_T("isProcessRunning: kill check for processName: %s, pid: %ld, rc: %d"), processName, lpid, rc));

             closedir(dir);
             TRACE_GLOBAL(Trace::Fatal, (_T("isProcessRunning result: %s"), res ? "YES" : "NO"));

            return res;
        }

        // Blocking call
        int DeviceProvisioning::execCmd(const char* cmd, int* prcRet, string* output)
        {
            FILE *prc = popen(cmd, "r");
            int res = STATUS_SUCCESS;
            int errCode = 0;

            if (prcRet)
                *prcRet = 0;

            if (output)
                *output = "";

            if(!prc)
            {
                res = STATUS_ERROR_OS;
                TRACE_GLOBAL(Trace::Fatal, (_T("cmdExec: unable to open a process for %s"), cmd));
            } else {
                char buf[256];
                while(fgets(buf, sizeof(buf), prc) != nullptr)
                {
                    if (output)
                        *output += buf;
                }
                errCode = pclose(prc);
                if (prcRet)
                    *prcRet = errCode;
            }
            if (0 != errCode)
            {
                res = STATUS_ERROR_OS;
                TRACE_GLOBAL(Trace::Fatal, (_T("cmdExec: process closed with %d"), errCode));
            }
            return  res;
        }

        void DeviceProvisioning::provisionCheckProcess(DeviceProvisioning* dp, JsonObject* rp) {
            // This method uses goto, hence all the function-scope variables must be declared here
            JsonObject entry;
            JsonArray provisionTypes;
            JsonObject response; // responding with an object holding a provisionTypes array (of entries)
            std::string logMessage;
            std::string inputPipe;
            std::string cmd;
            std::string output;
            std::vector<std::string> lines; // all filtered lines in the provision.properties file (see PROVISION_PROPERTIES_FILTER)
            std::string line;  // one line in the provisioning.properites file
            unsigned short int nbChecks = 0; // number of performed provision checks

            response.FromString("\"provisionTypes\": []");
            int cmdRet = STATUS_SUCCESS;
            int prcRet = 0;

            // Read /etc/provision.properties, which contains the supported provision types.
            // For each known provision type, call socprovisioning to check provisioning status.
            cmdRet = execCmd(PROVISION_PROPERTIES_FILTER.c_str(), &prcRet, &output);
            Utils::String::split(lines, output, "\n");
            for (std::vector<std::string>::iterator it = lines.begin() ; it != lines.end(); ++it) {
                char buffer[256] = {0};
                line = *it;
                output.clear();

                std::string processName;
                if (Utils::String::contains(line, "HARDWARE")) {
                    TRACE_GLOBAL(Trace::Information, (_T("Supported provision type: %s\n"), line.c_str()));
                    processName = provisionTypeToAppName("HARDWARE");
                    ++nbChecks;
                } else if (Utils::String::contains(line, "CRYPTANIUM")) {
                    TRACE_GLOBAL(Trace::Information, (_T("Supported provision type: %s\n"), line.c_str()));
                    processName = provisionTypeToAppName("CRYPTANIUM");
                    ++nbChecks;
                } else {
                    continue;
                }
                // check if socprovisioning process is running and return IN_PROGRESS status
                TRACE_GLOBAL(Trace::Information, (_T("Checking if %s is running\n"), processName.c_str()));
                if (isProcessRunning(processName)) {
                    TRACE_GLOBAL(Trace::Information, (_T("%s IS running\n"), processName.c_str()));
                    entry["provisionType"] = line;
                    entry["state"] = "IN_PROGRESS";
                    entry["reason"] = processName + " is already running";
                    provisionTypes.Add(entry);
                    continue;
                } else {
                    TRACE_GLOBAL(Trace::Information, (_T("%s is NOT running\n"), processName.c_str()));
                }

                if (Utils::String::contains(line, "HARDWARE") || Utils::String::contains(line, "CRYPTANIUM"))
                {
                    cmd = "/lib/rdk/launchSocProv.sh " + processName +" --status >" + AppNameToLogName(processName) + " 2>&1";
                    inputPipe = "/tmp/" + processName + "-fifo";
                }
                TRACE_GLOBAL(Trace::Fatal, (_T("Executing: %s\n"), cmd.c_str())); // TODO: remove Fatal
                cmdRet = execCmd(cmd.c_str(), &prcRet);
                if (cmdRet == STATUS_SUCCESS)
                {
                    FILE* pipe = fopen(inputPipe.c_str(), "r");
                    if(pipe)
                    {
                        while(!feof(pipe))
                        {
                            if (fgets(buffer, 256, pipe) != nullptr)
                            {
                                output += buffer;
                            }
                        }
                        fclose(pipe);
                        // ==== SUCCESS SCENARIO: BEGIN ====
                        JsonObject jsonOutput;
                        jsonOutput.FromString(output);
                        entry = jsonOutput["provisionTypes"].Array()[0].Object();
                        entry.ToString(logMessage);
                        provisionTypes.Add(entry);

                        TRACE_GLOBAL(Trace::Information, (_T("Got output: %s"), output.c_str()));
                        TRACE_GLOBAL(Trace::Information, (_T("Made entry: %s"), logMessage.c_str()));
                        // ==== SUCCESS SCENARIO: END ====
                    } else { // no pipe
                        entry["provisionType"] = line;
                        entry["status"] = std::to_string(STATUS_ERROR_NOFILE);
                        entry["reason"] = "Unable to read response fom: " + inputPipe + " with code: " + std::to_string(prcRet);
                        provisionTypes.Add(entry);
                    }
                } else { // cmdExec failed, sending a error response
                    entry["provisionType"] = line;
                    entry["status"] = std::to_string(cmdRet);
                    entry["reason"] = "Failed to execute: " + cmd + " with code: " + std::to_string(prcRet);
                    provisionTypes.Add(entry);
                }
            } // end looping over the props lines

            if (nbChecks == 0) {
                string reason = string("DeviceProvisioning::provisionCheck failed. No suppored provision types have been found in ") + string(PROVISION_PROPERTIES_FILE) +  string(" or the file is missing");
                entry["provisionType"] = "UNKNOWN";
                entry["status"] = std::to_string(STATUS_ERROR_NOFILE);
                entry["reason"] = reason;
                LOGERR("%s", reason.c_str());
                provisionTypes.Add(entry);
            }

            response["provisionTypes"] = provisionTypes;
            response.ToString(logMessage);
            if (rp) {
                *rp = response;
            }
            if (dp) {
                TRACE_GLOBAL(Trace::Information, (_T("Sending notification: %s %s\n"), DP_EVT_DID_FINISH_PROVISION_CHECK, logMessage.c_str()));
                dp->Notify(DP_EVT_DID_FINISH_PROVISION_CHECK, response);
            } else {
                TRACE_GLOBAL(Trace::Information, (_T("Skipping notification: %s\n"), DP_EVT_DID_FINISH_PROVISION_CHECK));
            }
        }

        bool  DeviceProvisioning::reprovisionProcess(const std::string& provisionType, DeviceProvisioning* dp, JsonObject* rp)
        {
            // This method uses goto, hence all the function-scope variables must be declared here

            JsonObject response; // responding with a single object
            std::string logMessage;
            std::vector<std::string> lines; // all filtered lines in the provision.properties file (see PROVISION_PROPERTIES_FILTER)
            std::string line; // in the provisioning.properites file
            std::string cmd;
            std::string output;
            std::string inputPipe;
            std::string logName;
            std::string processName;
            int cmdRet = STATUS_SUCCESS;
            int prcRet = 0;
            std::vector<std::string>::iterator it;
            bool bSupported = false;
            bool res = true;

            response["provisionType"] = provisionType;
            if (provisionType.empty())
            {
                response["status"] = std::to_string(STATUS_ERROR_PARAM);
                response["reason"] = "Provision type is not set";
                TRACE_GLOBAL(Trace::Error, (_T("DeviceProvisioning::reprovision failed. Provision type is not set")));
                res = false;
                goto end;
            }
            cmdRet = execCmd(PROVISION_PROPERTIES_FILTER.c_str(), &prcRet, &output);
            // Check that the given provisionType is supported.
            if (!prcRet == 0) {
                TRACE_GLOBAL(Trace::Error, (_T("DeviceProvisioning::provisionCheck reprovision. Unable to determine the supported provision types on device using %s."), PROVISION_PROPERTIES_FILE));
                response["status"] = std::to_string(STATUS_ERROR_NOFILE);
                response["reason"] = "Unable to to determine the supported provision types on device using " + string(PROVISION_PROPERTIES_FILE);
                res = false;
                goto end;
            }
            Utils::String::split(lines, output, "\n");

            for (it = lines.begin() ; it != lines.end(); ++it)
            {
                line = *it;
                if (Utils::String::equal(line, provisionType))
                {
                    bSupported = true;
                    break;
                }
            }

            if(!bSupported)
            {
                response["status"] = std::to_string(STATUS_ERROR_PARAM);
                response["reason"] = "Provision type " + provisionType + " is not supported";
                TRACE_GLOBAL(Trace::Error, (_T("DeviceProvisioning::reprovision failed. %s not supported"), provisionType.c_str()));
                res = false;
                goto end; // GO TO END
            }

            if(Utils::String::equal(line,"HARDWARE"))
            {
                processName = provisionTypeToAppName("HARDWARE");
            } else if(Utils::String::equal(line,"CRYPTANIUM")) {
                processName = provisionTypeToAppName("CRYPTANIUM");
            }
            logName = AppNameToLogName(processName);
            if (isProcessRunning(processName))
            {
                Utils::String::replace_substr(logName, processName, ("thunder-" + processName));
                TRACE_GLOBAL(Trace::Fatal, (_T("Another %s process is running. Starting reprovisioning anyway, logged to %s"), processName.c_str(), logName.c_str()));
            }

            cmd = "/lib/rdk/launchSocProv.sh " + processName +" --reprovision >" + logName + " 2>&1";
            inputPipe = "/tmp/" + processName + "-fifo";

            TRACE_GLOBAL(Trace::Information, (_T("Executing: %s\n"), cmd.c_str()));
            cmdRet = execCmd(cmd.c_str(), &prcRet);
            response["status"] = std::to_string(cmdRet);

            //TODO: ensure a well-formed JSON in all cases
            if (cmdRet == STATUS_SUCCESS)
            {
                char buffer[256] = {0};
                std::string output;
                FILE* pipe = fopen(inputPipe.c_str(), "r");

                if(pipe)
                {
                    while(!feof(pipe))
                    {
                        if(fgets(buffer,256,pipe) != nullptr)
                        {
                            output += buffer;
                        }
                    }
                    fclose(pipe);
                    // ==== SUCCESS SCENARIO: BEGIN ====
                    response.FromString(output);
                    response.ToString(logMessage);
                    TRACE_GLOBAL(Trace::Information, (_T("Got output: %s"), output.c_str()));
                    TRACE_GLOBAL(Trace::Information, (_T("Made entry: %s"), logMessage.c_str()));
                    // Clear auth token after successful hardware reprovisioning
                    std::string securityToken;
                    Utils::SecurityToken::getSecurityToken(securityToken);
                    if(Utils::String::equal(line,"HARDWARE"))
                    {
                        std::string authHeader;
                        if (!securityToken.empty())
                        {
                            authHeader = std::string(CURL_AUTH);
                            Utils::String::replace_substr(authHeader, "$TOKEN", securityToken);
                        }

                        cmd = string(CURL_CMD);
                        cmd += authHeader;
                        cmd +=  string(AUTHSERVICE_CLEAR_AUTH_TOKEN);
                        cmd += SERVICE_API_URL;
                        cmdRet = execCmd(cmd.c_str(), &prcRet);
                        TRACE_GLOBAL(Trace::Information, (_T("Auth token cleanup finished with %d, process code %d"), cmdRet, prcRet));

                        cmd = string(CURL_CMD);
                        cmd += authHeader;
                        cmd += string(AUTHSERVICE_CLEAR_SESSION_TOKEN);
                        cmd += SERVICE_API_URL;
                        cmdRet = execCmd(cmd.c_str(), &prcRet);
                        TRACE_GLOBAL(Trace::Information, (_T("Service session token cleanup finished with %d, process code %d"), cmdRet, prcRet));

                        cmd = string(CURL_CMD);
                        cmd += authHeader;
                        cmd += string(AUTHSERVICE_CLEAR_SERVICE_ACCESS_TOKEN);
                        cmd += SERVICE_API_URL;
                        cmdRet = execCmd(cmd.c_str(), &prcRet);
                        TRACE_GLOBAL(Trace::Information, (_T("Service access cleanup finished with %d, process code %d"), cmdRet, prcRet));
                    }
                    res = true;
                    // ==== SUCCESS SCENARIO: END ====
                } else { // no pipe
                    response["provisionType"] = line;
                    response["status"] = std::to_string(STATUS_ERROR_NOFILE);
                    response["reason"] = "Unable to read response fom: " + inputPipe + " with code: " + std::to_string(prcRet);
                    res = false;
                }
            } else { // cmdExec failed, sending a error response
                response["status"] = std::to_string(cmdRet);
                response["reason"] = "Reprovisioning command failed with " + std::to_string(cmdRet) + ", process error: " + std::to_string(prcRet);
                TRACE_GLOBAL(Trace::Error, (_T("Reprovisioning command failed with %d, process error: %d"), cmdRet, prcRet));
                res = false;
                goto end;
            }

            end:/*THE END*/
            response.ToString(logMessage);
            if (rp) {
                *rp = response;
            }
            if (dp) {
                TRACE_GLOBAL(Trace::Information, (_T("Sending notification: %s %s\n"), DP_EVT_DID_FINISH_REPROVISION, logMessage.c_str()));
                dp->Notify(DP_EVT_DID_FINISH_REPROVISION, response);
            } else {
                TRACE_GLOBAL(Trace::Information, (_T("Skipping notification: %s\n"), DP_EVT_DID_FINISH_REPROVISION));
            }
            return res;
        }

        void DeviceProvisioning::provisionRefreshProcess(const std::string appName)
        {
            std::string output;
            int prcRet = 0;
            std::string prcRet_lbl = appName + "/prcRet";

            std::string cmd(LAUNCH_SOCPROV_CMD);
            Utils::String::replace_substr(cmd, "#APP_NAME", appName);
            Utils::String::replace_substr(cmd, "#LOG_NAME", AppNameToLogName(appName));
            TRACE(Trace::Fatal, (_T("Executing external command: %s"), cmd.c_str()));
            execCmd(cmd.c_str(), &prcRet, &output);
            _eventLock.Lock();
            _reg->setVal(prcRet_lbl, prcRet);
            _eventLock.Unlock();
        }

        void DeviceProvisioning::getPrimaryDrmDir(std::string& drmDir)
        {
            bool fSuccess = getDrmDir("HARDWARE", drmDir);
            if (!fSuccess)
            {
                TRACE_GLOBAL(Trace::Fatal, (_T("Unable to read primary drm dir from %s; assuming /opt/drm"), PROVISION_PROPERTIES_FILE));
                drmDir = "/opt/drm";
            }
        }

        bool DeviceProvisioning::getDrmDir(const std::string& provisionType, std::string& drmDir)
        {
            std::string output;
            int cmdRet = STATUS_SUCCESS;
            int prcRet = 0;

            std::string getDrmDirCmd(GET_DRM_DIR_CMD);
            Utils::String::replace_substr(getDrmDirCmd, "#PROVISION_TYPE", provisionType);

            cmdRet = execCmd(getDrmDirCmd.c_str(), &prcRet, &output);
            bool fSuccess = (cmdRet == STATUS_SUCCESS && output.at(0) == '/');

            if (fSuccess)
            {
                if (output.back() == '\n')
                    output.pop_back();
                drmDir = output;
            } else {
                TRACE_GLOBAL(Trace::Error, (_T("Unable to read primary drm dir from %s; assuming /opt/drm"), PROVISION_PROPERTIES_FILE));
                drmDir = "/opt/drm";
            }
            return fSuccess;
        }

        // Quick-check, it can't tell whether the provisioned packages are healthy
        // provisionType - string, as described in /etc/provision.properties, e.g. HARDWARE, CRYPTANIUM
        bool DeviceProvisioning::deviceProvisioned(const std::string& provisionType)
        {
            bool result = false;
            std::stringstream metaFile, provisionListFile;
            std::string drmDir;

            bool fSuccess =  getDrmDir(provisionType, drmDir);
            if (fSuccess)
            {
                metaFile << drmDir << "/" << "meta.txt"; // V2 manifest
                provisionListFile << drmDir << "/" << "provisionList.txt"; // V1 manifest
                if (Utils::fileExists(metaFile.str().c_str())
                ||  Utils::fileExists(provisionListFile.str().c_str()))
                {
                    result = true;
                }
            }
            return result;
        }

        std::vector<std::string>& DeviceProvisioning::getSupportedProvisionTypes()
        {
            std::string output;
            static std::vector<std::string> s_provisionTypes;
            int cmdRet = STATUS_SUCCESS;
            int prcRet = 0;
            if (s_provisionTypes.empty())
            {
                cmdRet = execCmd(GET_PROVISION_TYPES_CMD.c_str(), &prcRet, &output);
                if (cmdRet == STATUS_SUCCESS && prcRet == 0)
                {
                    if (output.back() == '\n')
                        output.pop_back();

                    Utils::String::split(s_provisionTypes, output, " ");
                }
            } else {
                TRACE_GLOBAL(Trace::Information, (_T("Supported provision types acquired using the memorized value")));
            }
            return s_provisionTypes;
        }

        void DeviceProvisioning::getProvisionedTypes(std::vector<std::string>& provisionedTypes)
        {
            // Checking the supported types of provisioning for which the device is actually provisioned
            provisionedTypes = getSupportedProvisionTypes();
            std::vector<std::string>::const_iterator pt = provisionedTypes.begin();
            while (pt != provisionedTypes.end())
            {
                if (deviceProvisioned(*pt))
                {
                    TRACE_GLOBAL(Trace::Information, (_T("%s: device provisioned"), (*pt).c_str()));
                    ++pt;
                } else {
                    TRACE_GLOBAL(Trace::Fatal, (_T("%s: device is NOT provisioned"), (*pt).c_str()));
                    pt = provisionedTypes.erase(pt);
                }
            }
        }

        bool DeviceProvisioning::supported(const std::string& provisionType)
        {
            return (!provisionType.empty() && Utils::String::contains(getSupportedProvisionTypes(), provisionType));
        }

        void DeviceProvisioning::fulfillPrecondition(const std::string& provisionType, const std::string& reason)
        {
            std::vector<string> provisionTypes;
            provisionTypes.push_back(provisionType);
            fulfillPrecondition(provisionTypes, reason);
        }

        void DeviceProvisioning::fulfillPrecondition(/*const*/ std::vector<string>& provisionTypes, const std::string& reason)
        {
            // Ignoring CRYPTANIUM until OCDM is updated to process provisioning notifications and ignore unproper provisioning types itself
            std::vector<std::string>::const_iterator pt = provisionTypes.begin();
            while (pt != provisionTypes.end())
            {
                if (*pt == "CRYPTANIUM")
                {
                    TRACE(Trace::Warning, (_T("CRYPTANIUM provision type is not supposed to fulfill the PROVISIONING precondition. Ignored.")));
                    pt = provisionTypes.erase(pt);
                } else {
                    ++pt;
                }
            }

            if (_subSystem != nullptr && !provisionTypes.empty()) {
                Core::Sink<RPC::StringIterator> information (provisionTypes);
                _subSystem->Set(PluginHost::ISubSystem::PROVISIONING, &information);

                // notifying observers
                _eventLock.Lock();
                std::list<Exchange::IProvisioning::INotification*>::const_iterator client = _observers.begin();
                while(client != _observers.end()) {
                    (*client)->Provisioned(&information);
                    client++;
                }
                _eventLock.Unlock();

                std::string provisionTypesStr;
                Utils::String::implode(provisionTypes, provisionTypesStr);
                //TRACE(Trace::Information, (_T("PROVISIONING precondition fulfilled for %s; reason: %s."), provisionTypesStr.c_str(), reason.c_str()));
                LOGWARN("PROVISIONING precondition fulfilled for %s; reason: %s.", provisionTypesStr.c_str(), reason.c_str());
            }
        }

        // Time allotted by FKPS to make a fetch call for this ProvisionType
        uint64_t DeviceProvisioning::getRefreshEpoch(const std::string& provisionType)
        {
            uint64_t res = 0;
            std::string drmDir;
            std::string appName = provisionTypeToAppName(provisionType).c_str();
            if (_reg->contains(Reg::path({RTI, appName}, REFRESH_TIME_LBL)))
            {
                res = _reg->getU64Val(Reg::path({RTI, appName}, REFRESH_TIME_LBL));
                //TODO: remove
                TRACE(Trace::Fatal, (_T("Got refresh time from reg, %lld"), res));
            } else { // fallback method
                if(getDrmDir(provisionType, drmDir))
                {
                    res = DeviceInfo::getRefreshEpochFromFile(drmDir);
                    //TODO: remove
                    TRACE(Trace::Fatal, (_T("Got refresh time from file or assumed the current time: %lld"), res));
                } else {
                    TRACE(Trace::Error, (_T("Could not get refresh time for %s because runtime information is not present and drm dir was not found. Assuming 0"), appName));
                }
            }
            return res;
        }

        void DeviceProvisioning::setRefreshEpoch(uint64_t refreshEpoch, const std::string& provisionType)
        {
            std::string drmDir;
            std::string appName = provisionTypeToAppName(provisionType).c_str();
            std::string regPath = Reg::path({RTI, appName}, REFRESH_TIME_LBL);
            if (_reg->contains(regPath))
            {
                _reg->setVal(Reg::path({RTI, appName}, REFRESH_TIME_LBL), refreshEpoch);
            } else {
                TRACE(Trace::Error, (_T("Reg path %s is missing in the registry"), regPath.c_str()));
            }
            if(getDrmDir(provisionType, drmDir))
            {
                DeviceInfo::putRefreshEpochToFile(refreshEpoch, drmDir);
            } else {
                TRACE_GLOBAL(Trace::Error, (_T("Unable to read primary drm dir from %s"), provisionType.c_str()));
            }
        }

        int DeviceProvisioning::getBackoffInfo(const std::string& provisionType, uint64_t& prefetchFailCount, uint64_t& prefetchFailEpoch)
        {
            int res = BACKOFF_INFO_UNAVAILABLE;
            std::string drmDir;
            std::string appName = provisionTypeToAppName(provisionType).c_str();

            if (   _reg->contains(Reg::path({RTI, appName}, PREFETCH_FAIL_COUNT_LBL))
                && _reg->contains(Reg::path({RTI, appName}, PREFETCH_FAIL_EPOCH_LBL)))
            {
                prefetchFailCount = _reg->getU64Val(Reg::path({RTI, appName}, PREFETCH_FAIL_COUNT_LBL));
                prefetchFailEpoch = _reg->getU64Val(Reg::path({RTI, appName}, PREFETCH_FAIL_EPOCH_LBL));
                res = BACKOFF_INFO_FROM_RTI;
            } else { // fallback method
                if(getDrmDir(provisionType, drmDir))
                {
                    res = DeviceInfo::getBackoffInfoFromFile(drmDir, prefetchFailCount, prefetchFailEpoch);
                    if (Utils::adjustBackoff(prefetchFailCount, prefetchFailEpoch))
                    {
                        TRACE(Trace::Fatal, (_T("Backoff info adjusted: failure count: %lld, failure epoch: %lld"), prefetchFailCount, prefetchFailEpoch));
                    }
                } else {
                    TRACE(Trace::Error, (_T("Could not get backoff info for %s because runtime information is not present and drm dir was not found.")));
                }
            }
            return  res;
        }

        void DeviceProvisioning::onSocProvTimer(const std::string& provisionType, TpTimer* timer) //TODO: const timer
        {
            std::string appName = provisionTypeToAppName(provisionType).c_str();
            std::string exitMessage;
            uint64_t refreshEpoch = getRefreshEpoch(provisionType); // Time allotted by FKPS to make a fetch call for this ProvisionType
            uint64_t currentEpoch = Utils::getCurrentEpoch(); // Time now
            uint64_t checkBackDelaySec = TIMER_DEFAULT_CHECK_BACK_SECONDS;
            uint64_t min_wait = 29; // sec; minimum waiting time between consecutive invocations to avoid speed-loops and possible collisions
            std::string lastCall_lbl = Reg::path({DP, appName}, LASTCALL_LBL);
            std::string appRti;
            int backoffInfoRes = BACKOFF_INFO_UNAVAILABLE;
            int64_t powerStateAdjustmentValue = 0; //updated in calcWaitSeconds()

            // Lambdas: BEGIN
            auto wokeupRecently = [&]() {
                return ((currentEpoch - _wakeupTime) < 120); // "recently" is within 2 mins after wake up, otherwise we should have passed the "peak" call window for FKPS and hence there is no need to shift the call
            };

            auto calcWaitSeconds = [&]()
            {
                uint64_t prefetchFailCount = 0;
                uint64_t prefetchFailEpoch = 0;
                uint64_t backoffExpiration = 0;
                uint64_t lastCallEpoch =  _reg->contains(lastCall_lbl) ? _reg->getU64Val(lastCall_lbl) : 0; // Time when the previous call was made for this ProvisionType
                int64_t remainingSeconds = (refreshEpoch - currentEpoch);

                if (remainingSeconds <= 0) // it's time to run but checking backoff before that
                {
                    std::string backoffMessage;
                    uint64_t backoffIntervalMax = Utils::fibo(31);

                    backoffInfoRes = getBackoffInfo(provisionType, prefetchFailCount, prefetchFailEpoch);
                    if (backoffInfoRes == BACKOFF_INFO_UNAVAILABLE)
                    {
                        TRACE(Trace::Fatal, (_T("Could not get backoff info, assuming no backoff")));
                    } else if (backoffInfoRes == BACKOFF_INFO_FROM_FILE) {
                        TRACE(Trace::Information, (_T("Got backoff info from file")));
                    }
                    if (_reg->contains(Reg::path({RTI, appName}, BACKOFF_INTERVAL_MAX_LBL)))
                    {
                        backoffIntervalMax = _reg->getU64Val(Reg::path({RTI, appName}, BACKOFF_INTERVAL_MAX_LBL));
                    }
                    backoffExpiration = Utils::calcBackoffExpiration(prefetchFailCount, prefetchFailEpoch, backoffIntervalMax);
                    remainingSeconds = (backoffExpiration - currentEpoch);
                }
                remainingSeconds += ((currentEpoch - lastCallEpoch) < min_wait) ? min_wait : 0; // If we made another call recently, then we also add min_wait time to avoid speed-loops
                remainingSeconds = clamp(remainingSeconds, (int64_t)0, remainingSeconds); // if refresh-time is so back-dated that it becomes negative, settting it to 0 and running right now
                // Accounting for powerstate change
                if (wokeupRecently())
                {
                    _wakeupTime = 0;
                    powerStateAdjustmentValue = Utils::getRandomValue(0, POWERMODE_CHANGED_DELAY_SEC);
                    remainingSeconds += powerStateAdjustmentValue;
                    TRACE(Trace::Fatal, (_T("Refresh call time has been adjusted for %llu secods because the device has just woken up"), powerStateAdjustmentValue));
                }

                //TODO: change to Trace::Information
                TRACE(Trace::Fatal, (_T("refreshEpoch      : %llu"), refreshEpoch));
                TRACE(Trace::Fatal, (_T("currentEpoch      : %llu"), currentEpoch));
                TRACE(Trace::Fatal, (_T("lastCallEpoch     : %llu"), lastCallEpoch));
                TRACE(Trace::Fatal, (_T("remainingSeconds  : %lld"), remainingSeconds));
                if (backoffInfoRes != BACKOFF_INFO_UNAVAILABLE)
                {
                    TRACE(Trace::Fatal, (_T("prefetchFailCount : %llu"), prefetchFailCount));
                    TRACE(Trace::Fatal, (_T("prefetchFailEpoch : %llu"), prefetchFailEpoch));
                    TRACE(Trace::Fatal, (_T("backoffExpiration : %llu"), backoffExpiration));
                } else {
                    TRACE(Trace::Fatal, (_T("backoff info not checked because refresh time is not expired ")));
                }
                return remainingSeconds;
            };

            auto appShouldExit = [&](std::string& reason)
            {
                reason = "No action needed";
                return false;
            };
            // Lambdas: END

            gTimerLock.Lock();
            //TODO: change to Trace::Information
            TRACE(Trace::Fatal, (_T("Call for %s: begin"), provisionType.c_str()));
            int64_t waitSeconds = calcWaitSeconds();

            if (wokeupRecently() && (powerStateAdjustmentValue > 0))
            {
                int64_t newRefreshEpoch = currentEpoch + waitSeconds;
                TRACE(Trace::Fatal, (_T("Adjusting the stored %s refresh time from %lld to %lld"),provisionType.c_str(), refreshEpoch, newRefreshEpoch ));
                setRefreshEpoch(newRefreshEpoch, provisionType);
            }

            if (waitSeconds > 0)
            { // A0: not time to run
                TRACE(Trace::Fatal, (_T("Setting %s(%p) timer interval: %lld seconds"), provisionType.c_str(), timer, waitSeconds));
                // set the timer for the remaining time (based on refresh-time)
                timer->setInterval(1000 * waitSeconds);
            } else { // A1: it's time to run
                if (isProcessRunning(appName))
                // check if we need to signal the running process to exit
                // if yes, then set the exit flag and check back at the session deadline
                // If not, then  just check back at the session deadline
                { // B1: process is running
                    TRACE(Trace::Fatal, (_T("%s is running right now"), appName.c_str()));
                    // running process stats
                    appRti = _reg->toMultilineString(Reg::path({RTI, appName},""));
                    //TODO: change to Trace::Information
                    TRACE(Trace::Fatal, (_T("Runtime information:\n %s"), appRti.c_str()));
                    if(appShouldExit(exitMessage))
                    { // C1
                        _reg->setVal(Reg::path({DP, appName}, APP_SHOULD_EXIT_LBL), true);
                        _reg->setVal(Reg::path({DP, appName}, APP_SHOULD_EXIT_MESSAGE_LBL), exitMessage);
                    }
                    // set the timer after the session deadline
                    if (_reg->contains(Reg::path({RTI, appName}, SESSION_DEADLINE_EPOCH_LBL)))
                    {
                        uint64_t delaySec = _reg->getU64Val(Reg::path({RTI, appName}, SESSION_DEADLINE_EPOCH_LBL)) - currentEpoch;
                        if (delaySec > 0)
                        {
                            checkBackDelaySec = delaySec;
                        } else {
                            TRACE(Trace::Fatal, (_T("Session deadline expired for %s!"), appName.c_str()));
                        }
                    } else {
                        TRACE(Trace::Fatal, (_T("No session deadline info available for %s"), appName.c_str()));
                    }
                } else { // B0: process is not running
                    // start a new process
                    // check back after min-time
                    //TODO: change to Trace::Information
                    TRACE(Trace::Fatal, (_T("Doing cleanup after the previous call to %s "), appName.c_str()));
                    auto trunk = _socprovThreads.find(provisionType);
                    if (trunk != _socprovThreads.end())
                    {
                        if(trunk->second->joinable())
                        {
                            trunk->second->join();
                            delete trunk->second;
                            trunk->second = nullptr;
                        }
                        _socprovThreads.erase(trunk);
                    }
                    //TODO: change to Trace::Information
                    TRACE(Trace::Fatal, (_T("Calling %s "), appName.c_str()));
                    _reg->setVal(lastCall_lbl, (uint64_t)currentEpoch);
                    auto thd = new std::thread(&DeviceProvisioning::provisionRefreshProcess, this, appName);
                    _socprovThreads.insert( {provisionType, thd} );
                    // set a short check back time
                }
                TRACE(Trace::Fatal, (_T("%s: checking back in %lld sec"), appName.c_str(), checkBackDelaySec));
                timer->setInterval(1000 * checkBackDelaySec);
            }
            //TODO: change to Trace::Information
            TRACE(Trace::Fatal, (_T("Call for %s: end"), provisionType.c_str()));
            gTimerLock.Unlock();
        }

        void DeviceProvisioning::onOCDMTimer()
        {
            std::lock_guard<std::mutex> guard(_callMutex);
            static int remainingAttempts = MAX_OCDM_TIMER_WAIT_ATTEMPTS;
            bool pluginActivated = Utils::isPluginActivated(OCDM_CALLSIGN);
            if (pluginActivated)
            {
                if (_ocdmTimer.isActive()) {
                    _ocdmTimer.stop();
                    TRACE(Trace::Information, (_T("ODCM monitoring stopped.")));
                }
                // Checking the supported types of provisioning for which the device is actually provisioned
                std::vector<std::string> provisionedTypes;
                getProvisionedTypes(provisionedTypes);

                if (!provisionedTypes.empty())
                {
                    fulfillPrecondition(provisionedTypes, "OCDM plugin activation");
                } else {
                    TRACE(Trace::Fatal, (_T("Device does not have any DRM packages. PROVISIONING precondition could not be fulfilled yet. Waitig for the signal from SocProvisioning")));
                }
            } else {
                --remainingAttempts;
                if (remainingAttempts > 0)
                {
                    TRACE(Trace::Warning, (_T("Plugin %s is not activated, waiting for %d msec (%d attemps remaning). "), OCDM_CALLSIGN, OCDM_WAIT_TIME_IN_MILLISECONDS, remainingAttempts));
                } else {
                    if (_ocdmTimer.isActive()) {
                        _ocdmTimer.stop();
                        TRACE(Trace::Fatal, (_T("Timed out waiting for OCDM activation. Giving up")));
                    }
                }
            }
        }

        //
        /// Internal methods end

        /// Registered methods begin
        //
        uint32_t DeviceProvisioning::provisionCheckAsync(const JsonObject &parameters, JsonObject &response) {

            if (_executionThread.joinable())
            {
                _executionThread.join();
            }
            _executionThread = std::thread(provisionCheckProcess, this, &response);
            returnResponse(true);
        }

        uint32_t DeviceProvisioning::provisionCheckSync(const JsonObject &parameters, JsonObject &response) {
            provisionCheckProcess(nullptr, &response);
            returnResponse(true);
        }

        uint32_t DeviceProvisioning::reprovisionAsync(const JsonObject& parameters, JsonObject& response)
        {
            std::string provisionType;
            std::string supportedTypes;
            std::stringstream errorMsg;
            std::string errorMsgStr;
            bool res = false;

            if (parameters.HasLabel("provisionType"))
            {
                getStringParameter("provisionType", provisionType);
            }
            if (supported(provisionType))
            {
                if (_executionThread.joinable())
                {
                    _executionThread.join();
                }
                _executionThread = std::thread(reprovisionProcess, provisionType, this, &response);
                res = true;
            } else {
                errorMsg << "Invalid provisionType param: " << provisionType << ". Accepting " << Utils::String::implode(getSupportedProvisionTypes(), supportedTypes);
                errorMsgStr = errorMsg.str();
                response["message"] = errorMsgStr;
                TRACE(Trace::Fatal, (_T("%s"), errorMsgStr.c_str()));
            }
            returnResponse(res);
        }

        uint32_t DeviceProvisioning::reprovisionSync(const JsonObject& parameters, JsonObject& response)
        {
            std::string provisionType;
            std::string supportedTypes;
            std::stringstream errorMsg;
            std::string errorMsgStr;
            bool res = false;
            if (parameters.HasLabel("provisionType"))
            {
                getStringParameter("provisionType", provisionType);
            }
            if (supported(provisionType))
            {
                res = reprovisionProcess(provisionType, nullptr, &response);
            } else {
                errorMsg << "Invalid provisionType param: " << provisionType << ". Accepting " << Utils::String::implode(getSupportedProvisionTypes(), supportedTypes);
                errorMsgStr = errorMsg.str();
                response["message"] = errorMsgStr;
                TRACE(Trace::Fatal, (_T("%s"), errorMsgStr.c_str()));
            }
            returnResponse(res);
        }

        uint32_t DeviceProvisioning::notifyProvisioningSync(const JsonObject& parameters, JsonObject& response)
        {
            std::string provisionType;
            std::string supportedTypes;
            std::stringstream errorMsg;
            std::string errorMsgStr;
            bool res = false;
            bool deprecated = true;
            if (parameters.HasLabel("provisionType"))
            {
                getStringParameter("provisionType", provisionType);
            }
            if (supported(provisionType))
            {
                if (deviceProvisioned(provisionType))
                {
                    if (provisionType == "CRYPTANIUM")
                    {
                        // Ignoring CRYPTANIUM until OCDM is updated to process provisioning notifications and ignore unproper provisioning types itself
                        TRACE(Trace::Warning, (_T("CRYPTANIUM provision type is not supposed to fulfill the PROVISIONING precondition. Ignored.")));
                        res = false;
                    } else {
                        fulfillPrecondition(provisionType, "notification from SocProvisioning");
                        res = true;
                    }
                } else {
                    errorMsg << "Provision notification for " << provisionType << " ignored. Device is NOT provisioned with that type yet";
                    errorMsgStr = errorMsg.str();
                    response["message"] = errorMsgStr;
                    TRACE(Trace::Fatal, (_T("%s"), errorMsgStr.c_str()));
                }
            } else {
                errorMsg << "Invalid provisionType param: " << provisionType << ". Accepting " << Utils::String::implode(getSupportedProvisionTypes(), supportedTypes);
                errorMsgStr = errorMsg.str();
                response["message"] = errorMsgStr;
                TRACE(Trace::Fatal, (_T("%s"), errorMsgStr.c_str()));
            }
            response["deprecated"] = deprecated;
            returnResponse(res);
        }

        uint32_t DeviceProvisioning::getNetflixEsnSync(const JsonObject& parameters, JsonObject& response)
        {
            bool res = false;
            std::string output;
            int prcRet = 0;
            std::stringstream errorMsg, netflixEsnFile;
            std::string errorMsgStr;
            std::string getNetflixEsnCmd(GET_NETFLIX_ESN_CMD);

            Utils::String::replace_substr(getNetflixEsnCmd, "#PRIMARY_DRM_DIR", _primaryDrmDir);
            Utils::String::replace_substr(getNetflixEsnCmd, "#NETFLIX_ESN_FILENAME", NETFLIX_ESN_FILENAME);

            netflixEsnFile << _primaryDrmDir << "/" << NETFLIX_ESN_FILENAME;

            if (Utils::fileExists(netflixEsnFile.str().c_str()))
            {
                execCmd(getNetflixEsnCmd.c_str(), &prcRet, &output);
                TRACE(Trace::Information, (_T("Got output: %s"), output.c_str()));
                JsonObject jsonOutput(output);
                std::string esn = jsonOutput.Get("esn").String();

                if (esn == "null")
                {
                    errorMsg << "Unable to get ESN (damaged file " << netflixEsnFile.str() << "). The following command failed: '" << GET_NETFLIX_ESN_CMD << "'. Output: " << output;
                    errorMsgStr = errorMsg.str();
                    Utils::String::replace_substr(errorMsgStr, "\n", "");
                    response["message"] = errorMsgStr;
                } else {
                    res = true;
                    response["esn"] = esn;
                }
            } else { // esn file does not exist
                errorMsg << "Unable to get ESN (missing file " << netflixEsnFile.str() << "). Netflix ESN is not available on this system";
                response["message"] = errorMsg.str();
            }
            returnResponse(res);
        }

        uint32_t DeviceProvisioning::getSOCIdSync(const JsonObject& parameters, JsonObject& response)
        {
            bool res = false;
            std::stringstream errorMsg;
            std::string message;
            std::vector<SEC_BYTE> SOCId;
            std::string SOCIdHex;

            if (_sec.get() == nullptr)
            {
                message = "Could not create money trace header - SecApi proc Handle not instantiated";
                TRACE(Trace::Fatal, (_T(message.c_str())));
            } else {
                try{
                    SOCId = Sec::SecApi::deviceId(_sec.get());
                    std::string SOCIdHex = Sec::Utils::bytes_to_hex(SOCId);
                    response["socid"] = SOCIdHex;
                    res = true;
                }
                catch (const Sec::SecApiException& e) {
                    errorMsg << "Sec API getting SOC id failed exception caught - " << e.what();
                    message = errorMsg.str();
                    TRACE(Trace::Fatal, (_T(message.c_str())));
                    res = false;
                }
            }
            if (!message.empty())
            {
                response["message"] = message;
            }
            returnResponse(res);
        }

        uint32_t DeviceProvisioning::notifyRuntimeInformationSync(const JsonObject& parameters, JsonObject& response)
        {
            bool res = false;
            std::string provisionType;
            std::string appName = "UNKNOWN";

            //TODO: remove
            std::string rtiStr;
            parameters.ToString(rtiStr);
            TRACE(Trace::Fatal, (_T("Got runtime information: %s"),rtiStr.c_str()));

            #define TAKE_PARAM_TO_REG(paramName, type, dataType) \
            if (parameters.HasLabel(paramName)) \
            { \
                dataType var;\
                get##type##Parameter(paramName, var); \
                _reg->setVal(std::string(RTI) + "/" + appName + "/" + std::string(paramName), var); \
            }

            if (parameters.HasLabel(PROVISION_TYPE_LBL))
            {
                getStringParameter("provisionType", provisionType);
                appName = provisionTypeToAppName(provisionType);
            }
            _reg->eraseByPrefix(Reg::path({RTI, appName}, "")); // deleting the previous RTI for this app
            // Example: if "parameters" has "provisionType" label with value "HARDWARE",
            // then we store a pair like : ("rti/socprovisioning/provisionType", "HARDWARE") in _reg
            // for crypto it would store a value like this: ("rti/socprovisioning-crypto/bootupMode", true)
            TAKE_PARAM_TO_REG(BOOTUP_LBL, Bool, bool)
            TAKE_PARAM_TO_REG(CLOCKSET_LBL, Bool, bool)
            TAKE_PARAM_TO_REG(DEVICE_CONNECTED_LBL, Bool, bool)
            TAKE_PARAM_TO_REG(FETCH_CALLED_LBL, Bool, bool)
            TAKE_PARAM_TO_REG(PROVISION_CALLED_LBL, Bool, bool)
            TAKE_PARAM_TO_REG(REFRESH_TIME_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(TIME_WAIT_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(PREFETCH_FAIL_EPOCH_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(PREFETCH_FAIL_COUNT_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(SESSION_START_EPOCH_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(SESSION_DEADLINE_EPOCH_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(BACKOFF_INTERVAL_MAX_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(BACKOFF_EXPIRATION_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(CTL_RESULT_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(FETCH_RESULT_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(PROVISION_RESULT_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(HEALTHCHECK_RESULT_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(PROVISION_OPTIONS_LBL, Number, uint64_t)
            TAKE_PARAM_TO_REG(PROVISION_TYPE_LBL, String, std::string)
            TAKE_PARAM_TO_REG(FINAL_LBL, Bool, bool)

            std::string appShouldExit_lbl = Reg::path({DP, appName}, APP_SHOULD_EXIT_LBL);
            std::string appShouldExitMessage_lbl = Reg::path({DP, appName}, APP_SHOULD_EXIT_MESSAGE_LBL);
            std::string appRtiFinal_lbl = Reg::path({RTI, appName}, FINAL_LBL);
            if (_reg->contains(appShouldExit_lbl))
            {
                response["shouldExit"] = _reg->getBoolVal(appShouldExit_lbl);
                if (_reg->contains(appShouldExitMessage_lbl))
                {
                    response["shouldExitMessage"] = _reg->getStdStrVal(Reg::path({DP, appName}, APP_SHOULD_EXIT_MESSAGE_LBL));
                }
            }
            // If this was a final call from SoC Provisioning, we trigger onSocProvTimer() so that the timers will be adjusted for the new refresh-time
            if (_reg->contains(appRtiFinal_lbl) && _reg->getBoolVal(appRtiFinal_lbl))
            {
                TRACE(Trace::Fatal, (_T("Triggering the SoC Provisioning timer after getting the final RTI block for %s"), provisionType.c_str()));
                for(auto entry: _socprovTimers)
                {
                    uint64_t delay = TIMER_INIT_DELAY_IN_MILLISECONDS;
                    if (provisionType == entry.first)
                    {
                        TRACE(Trace::Fatal, (_T("%s timer (%p) is set to %lld msec"), entry.first.c_str(), entry.second, delay));
                        entry.second->setInterval(delay);
                    }
                }
            }
            res = true;
            returnResponse(res);
        }

        uint32_t DeviceProvisioning::getApiVersionNumber(const JsonObject& parameters, JsonObject& response)
        {
            response["version"] = _apiVersionNumber;
            returnResponse(true);
        }

        uint32_t DeviceProvisioning::checkForUpdatesSync(const JsonObject& parameters, JsonObject& response)
        {
            TRACE(Trace::Fatal, (_T("Not implemented")));
            returnResponse(false);
        }

        uint32_t DeviceProvisioning::setAutoProvisionMode(const JsonObject& parameters, JsonObject& response)
        {
            TRACE(Trace::Fatal, (_T("Not implemented")));
            returnResponse(false);
        }

        uint32_t DeviceProvisioning::getAutoProvisionMode(const JsonObject& parameters, JsonObject& response)
        {
            TRACE(Trace::Fatal, (_T("Not implemented")));
            returnResponse(false);
        }
        //
        /// Registered methods end

    } // Plugin
} // WPEFramework
