#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

class tag {
public:
	tag(const std::string& name);
	~tag();

	template<typename T>
	void addAttribute(const std::string& name, const T& value);

	static void openFile(const std::string& filename);

protected:
	void prepend(std::ostream& str);

private:
	std::string m_name;
	std::stringstream m_childStream;
	std::map<std::string, std::string> m_attributes;

	static std::vector<tag*> m_tagStack;
	static std::ofstream* m_stream;
};

//////////////////////////////

template<typename T>
void tag::addAttribute(const std::string& name, const T& value) {
	std::stringstream ss;
	ss << value;
	m_attributes.insert(std::make_pair(name, ss.str()));
};
