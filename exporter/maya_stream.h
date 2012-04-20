#pragma once

#include <sstream>
#include <maya/MFnPlugin.h>

// uber-simple way of supporting endl (any other manipulator would end up behaving the same as endl, but who cares in this super-simple class)
typedef std::basic_ostream<char, std::char_traits<char> > cout_type;
typedef cout_type& (*endl_type)(cout_type&);

class maya_stream {
public:
	template<typename T>
	maya_stream& operator << (const T& val);

	maya_stream& operator<<(std::ostream&(*f)(std::ostream&));

	template<>
	maya_stream& maya_stream::operator << (const endl_type& val);


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

maya_stream& maya_stream::operator<<(std::ostream&(*f)(std::ostream&)) {
    if( f == std::endl ) {
        MGlobal::displayInfo( m_stream.str().c_str() );
		m_stream.str("");
    }

    return *this;
}
