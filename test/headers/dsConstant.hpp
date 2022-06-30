
#ifndef DSCONSTANTS_HPP_
#define DSCONSTANTS_HPP_

#include <string>
namespace device {

	class DSConstant  {
		private:

	bool enabled;   //!< Indicates the port or port attributes inheriting this class is enabled or not.

protected:
	int _id;        //!< Indicates the id of the instance inheriting this class.
	std::string _name;   //!< Indicates the name string of the instance inheriting this class.


	static bool isValid(int min, int max, int val) {
		return (val >= min && val < max);
	}

public:

	DSConstant() : enabled(false), _id(0), _name("_UNASSIGNED NAME_"){};


	DSConstant(const int id, const std::string &name) : enabled(false), _id(id), _name(name){};

	virtual ~DSConstant() {};

	virtual bool operator==(int id) const{
		return id == _id;
	}

	virtual int getId() const {
		return _id;
	};

	virtual const std::string & getName() const {
		return _name;
	}

	virtual const std::string & toString() const {
		return _name;
	}

	void enable() {
		enabled = true;
	}

	bool isEnabled() const{
		return enabled;
	}

};
}
#endif /* DSCONSTANT_HPP_ */

