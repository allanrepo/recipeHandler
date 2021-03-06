#ifndef __UTILITY__
#define __UTILITY__

#include <string>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <list>
#include <vector>
#include <fstream>



namespace CUtil
{
	class CSingleton
	{
	private:
		CSingleton(){}  // private so we can't instantiate
		virtual ~CSingleton(){} // private so we can't destroy
		CSingleton(const CSingleton&){} // make copy constructor private so we can't copy                    
		const CSingleton& operator=(const CSingleton&){ return *this; } // make = operator private so we can't copy

	public:
		static CSingleton& instance()
		{
			static CSingleton inst;
			return inst;
		}
	};

	// standard class version
	class CLog
	{
	private:
		std::stringstream m_stream;
		std::string m_file;

	public:
		CLog()  
		{
			immediate = true;
			enable = true;
			silent = false;
		}
		virtual ~CLog(){} 

	public:
		// properties
		bool immediate;
		bool enable;
		bool silent;

		void file(const std::string& file = ""){ m_file = file; }
		void clear(){ m_stream.str(std::string()); }
		void flush()
		{ 
			if (!silent) std::cout << m_stream.str(); 
			if (m_file.size())
			{
				std::fstream fs;
				fs.open(m_file.c_str(), std::fstream::out | std::fstream::app);
				if (fs.is_open()) fs << m_stream.str();
				fs.close();
			}
			clear(); 
		}


		template <class T>
		CLog& operator << (const T& s)
		{
			if (enable)
			{
				if(immediate && !silent) std::cout << s;
				else m_stream << s;
			}
			return *this;
		}
		template <class T>
		CLog& operator << (T& s)
		{
			if (enable)
			{
				if(immediate && !silent) std::cout << s;
				else m_stream << s;
			}
			return *this;
		}

		static std::ostream& endl(std::ostream &o)
		{
			o << std::endl;	
			return o;
		}
	};

	long toLong(const std::string& num);
	bool isNumber(const std::string& n);
	bool isInteger(const std::string& n);
};



#endif
