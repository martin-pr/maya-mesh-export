#pragma once

#include <sstream>
#include <maya/MFnPlugin.h>

class maya_stream {
public:
	template<typename T>
	maya_stream& operator << (const T& val);

	maya_stream& operator<<(std::ostream&(*f)(std::ostream&));

protected:
private:
	std::stringstream m_stream;
};

////////////////////////////////////

template<typename T>
maya_stream& maya_stream::operator << (const T& val) {
	m_stream << val;
	return *this;
};

// uber-simple way of supporting endl
maya_stream& maya_stream::operator<<(std::ostream&(*f)(std::ostream&)) {
    if( f == std::endl ) {
        MGlobal::displayInfo( m_stream.str().c_str() );
		m_stream.str("");
    }

    return *this;
}
