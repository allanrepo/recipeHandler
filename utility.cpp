#include <utility.h>


long CUtil::toLong(const std::string& n)
{
	std::stringstream a(n);
	long val;
	a >> val;    
	return val;
} 

bool CUtil::isNumber(const std::string& n)
{
	bool bDigit = false;
	bool bSign = false; 
	bool bDecimal = false;
	
	for (std::string::size_type i = 0; i < n.size(); i++)
	{
		switch( n[i] )
		{
			case '+':
			case '-':
			{
				if (bDigit) return false; // already found a numeral prior to sign
				if (i != 0) return false; // sign is not the first char
				if (bSign) return false; // found multiple sign
				bSign = true;
				break;
			}
			case '.':
			{
				if (bDecimal) return false; // found multiple decimal point
				bDecimal = true;
				bDigit = true;
				break;
			}
			default:
			{
				
				if (n[i] < '0' || n[i] > '9') return false; // any character that is not a number is found
				bDigit = true; // already found a valid digit				
			}
		}
	}

	return true;
}

bool CUtil::isInteger(const std::string& n)
{
	bool bDigit = false;
	bool bSign = false; 
	
	for (std::string::size_type i = 0; i < n.size(); i++)
	{
		switch( n[i] )
		{
			case '+':
			case '-':
			{
				if (bDigit) return false; // already found a numeral prior to sign
				if (i != 0) return false; // sign is not the first char
				if (bSign) return false; // found multiple sign
				bSign = true;
				break;
			}
			default:
			{				
				if (n[i] < '0' || n[i] > '9') return false; // any character that is not a number is found
				bDigit = true; // already found a valid digit				
			}
		}
	}

	return true;
}
