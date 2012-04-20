#include "tag.h"

using namespace std;

std::vector<tag*> tag::m_tagStack;
std::ofstream* tag::m_stream = NULL;

tag::tag(const std::string& name) : m_name(name) {
	assert(m_stream != NULL);
	m_tagStack.push_back(this);
}

tag::~tag() {
	assert(m_tagStack.back() == this);
	m_tagStack.pop_back();

	ostream* str;
	if(m_tagStack.size() == 0)
		str = m_stream;
	else
		str = &(m_tagStack.back()->m_childStream);

	prepend(*str);
	(*str) << "<" << m_name;
	for(std::map<std::string, std::string>::iterator i = m_attributes.begin(); i != m_attributes.end(); i++)
		(*str) << " " << i->first << "=\"" << i->second << "\"";
	
	if(m_childStream.str().length() == 0)
		(*str) << " />" << endl;
	else {
		(*str) << ">" << endl;
		(*str) << m_childStream.str();
		prepend(*str);
		(*str) << "</" << m_name << ">" << endl;
	}

	if(m_tagStack.size() == 0) {
		m_stream->close();
		delete m_stream;
		m_stream = NULL;
	}
}

void tag::prepend(ostream& str) {
	for(unsigned a=0;a<m_tagStack.size();a++)
		str << "  ";
}

void tag::openFile(const std::string& filename) {
	assert(m_stream == NULL);
	m_stream = new ofstream(filename.c_str());
}