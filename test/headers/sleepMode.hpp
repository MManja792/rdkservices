
#include <vector>

namespace device {

template <class T>
using List = std::vector<T>;


class DSConstant  {
	private:

	bool enabled;
	protected:
        int _id;
	std::string _name;
	static bool isValid(int min, int max, int val) {
                return (val >= min && val < max);
        }

public:
	DSConstant() : enabled(false), _id(0), _name("_UNASSIGNED NAME_"){};

};


class SleepMode : public DSConstant {

public:
        static const int kLightSleep;  //!< Indicates light sleep mode.
        static const int kDeepSleep;   //!< Indicates deep sleep mode.
        static const int kMax;         

        static SleepMode & getInstance(int id);
        static SleepMode & getInstance(const std::string &name);
        List<SleepMode> getSleepModes();
        SleepMode(int id);
        virtual ~SleepMode();

};

}
