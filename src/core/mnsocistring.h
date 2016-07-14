//
// Copyright (C) 2004-2008 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MNSOCISTRING_H_INCLUDED
#define MNSOCISTRING_H_INCLUDED

#include "soci-config.h"
#include <sqltypes.h>
#include <sql.h>
#include <vector>



class MNSociString
{
	public:
	static const int MNSOCI_SIZE = 257;

	MNSociString():m_iSize(MNSOCI_SIZE) { m_ptrCharData = new char[m_iSize]; m_ptrCharData[0] = '\0'; m_iIndicator = 0; }
	MNSociString(const char* ptrChar) :m_iSize(MNSOCI_SIZE) { m_ptrCharData = new char[m_iSize]; strcpy(m_ptrCharData, ptrChar); m_iIndicator = strlen(ptrChar); }
	MNSociString(const MNSociString& obj):m_iSize(obj.m_iSize) { m_ptrCharData = new char[m_iSize]; *this = obj; }
    virtual ~MNSociString() 
    {
        if (m_ptrCharData != NULL)  
        { 
            delete [] m_ptrCharData; 
            m_ptrCharData = NULL; 
        }
    }

	MNSociString& operator = (const MNSociString& obj)  { strcpy(m_ptrCharData, obj.m_ptrCharData); m_iIndicator = obj.m_iIndicator; m_iSize = obj.m_iSize; return *this; }
    MNSociString& operator = (char* ptrChar)            { strcpy(m_ptrCharData, ptrChar); m_iIndicator = strlen(ptrChar); return *this; }
    MNSociString& operator = (const char* ptrChar)      { strcpy(m_ptrCharData, ptrChar); m_iIndicator = strlen(ptrChar); return *this; }

	const int& getSize() const { return m_iSize; }

    char* m_ptrCharData;
    SQLLEN m_iIndicator;
	int m_iSize;
};

class MNSociText 
{
	public:	
		static const int MNSOCI_SIZE = 4002;

		MNSociText() :m_iSize(MNSOCI_SIZE) { m_ptrCharData = new char[m_iSize]; m_ptrCharData[0] = '\0'; m_iIndicator = 0; }
		MNSociText(const char* ptrChar) :m_iSize(MNSOCI_SIZE) { m_ptrCharData = new char[m_iSize]; strcpy(m_ptrCharData, ptrChar); m_iIndicator = strlen(ptrChar); }
		MNSociText(const MNSociText& obj):m_iSize(obj.m_iSize) { m_ptrCharData = new char[m_iSize]; *this = obj; }
		virtual ~MNSociText()
	    {
		  if (m_ptrCharData != NULL)
		  {
			delete[] m_ptrCharData;
			m_ptrCharData = NULL;
		  }
	   }

		MNSociText& operator = (const MNSociText& obj)  { strcpy(m_ptrCharData, obj.m_ptrCharData); m_iIndicator = obj.m_iIndicator; m_iSize = obj.m_iSize; return *this; }
		MNSociText& operator = (char* ptrChar)            { strcpy(m_ptrCharData, ptrChar); m_iIndicator = strlen(ptrChar); return *this; }
		MNSociText& operator = (const char* ptrChar)      { strcpy(m_ptrCharData, ptrChar); m_iIndicator = strlen(ptrChar); return *this; }

	const int& getSize() const { return m_iSize; }

	char* m_ptrCharData;
	SQLLEN m_iIndicator;
	int m_iSize;

};

class MNSociArrayString
{
public:
	MNSociArrayString(int iArraySize)
		:m_iSize(MNSociString::MNSOCI_SIZE),
		 m_iArraySize(iArraySize)
    { 
		m_ptrArrayCharData = new char[m_iSize * iArraySize];
        m_ptrArrayCharData[0] = '\0'; 
        m_iCurrentArrayInsertPosition = 0; 
        m_vecIndicators.resize(iArraySize, SQL_NULL_DATA);
    }

    ~MNSociArrayString()
    {
        if (m_ptrArrayCharData)
        {
            delete [] m_ptrArrayCharData;
            m_ptrArrayCharData = NULL;
        }
    }

    MNSociArrayString& operator = (const MNSociArrayString& obj); //create compile error when used

	void    push_back(const char* ptrChar) { strcpy(&m_ptrArrayCharData[m_iCurrentArrayInsertPosition * m_iSize], ptrChar); m_vecIndicators[m_iCurrentArrayInsertPosition] = ptrChar ? strlen(ptrChar) : SQL_NULL_DATA;  ++m_iCurrentArrayInsertPosition; }
	void    push_back(char* ptrChar) { strcpy(&m_ptrArrayCharData[m_iCurrentArrayInsertPosition * m_iSize], ptrChar); m_vecIndicators[m_iCurrentArrayInsertPosition] = ptrChar ? strlen(ptrChar) : SQL_NULL_DATA; ++m_iCurrentArrayInsertPosition; }
    
	char*   getValue(const int& iCurrentArrayReadPos) { return &m_ptrArrayCharData[iCurrentArrayReadPos * m_iSize]; }
    SQLLEN  getIndicator(const int& iCurrentArrayReadPos) { return m_vecIndicators[iCurrentArrayReadPos]; }

    char*       getArrayCharData()  { return m_ptrArrayCharData; }
    SQLLEN*     getArrayIndicators() { return (SQLLEN*)&m_vecIndicators[0]; }
    const int&  getArraySize()  const     { return m_iArraySize; }
    int         getCurrentInsertedElementCount() { return m_iCurrentArrayInsertPosition; }

    void        reset() {  m_iCurrentArrayInsertPosition = 0; }
	const int&   getStringSize() const { return m_iSize;  }

private:
    int     m_iArraySize;
    //starts with 0
    int     m_iCurrentArrayInsertPosition;

    char*   m_ptrArrayCharData;

    std::vector<SQLLEN> m_vecIndicators;
	int     m_iSize;
};

class MNSociArrayText 
{
	public:
		MNSociArrayText(int iArraySize )
			:m_iSize(MNSociText::MNSOCI_SIZE),
			m_iArraySize(iArraySize)
		{
			m_ptrArrayCharData = new char[m_iSize * iArraySize];
			m_ptrArrayCharData[0] = '\0';
			m_iCurrentArrayInsertPosition = 0;
			m_vecIndicators.resize(iArraySize, SQL_NULL_DATA);
		}

		~MNSociArrayText()
		{
			if (m_ptrArrayCharData)
			{
				delete[] m_ptrArrayCharData;
				m_ptrArrayCharData = NULL;
			}
		}

		MNSociArrayText& operator = (const MNSociArrayText& obj); //create compile error when used

		void    push_back(const char* ptrChar) { strcpy(&m_ptrArrayCharData[m_iCurrentArrayInsertPosition * m_iSize], ptrChar); m_vecIndicators[m_iCurrentArrayInsertPosition] = ptrChar ? strlen(ptrChar) : SQL_NULL_DATA;  ++m_iCurrentArrayInsertPosition; }
		void    push_back(char* ptrChar) { strcpy(&m_ptrArrayCharData[m_iCurrentArrayInsertPosition * m_iSize], ptrChar); m_vecIndicators[m_iCurrentArrayInsertPosition] = ptrChar ? strlen(ptrChar) : SQL_NULL_DATA; ++m_iCurrentArrayInsertPosition; }

		char*   getValue(const int& iCurrentArrayReadPos) { return &m_ptrArrayCharData[iCurrentArrayReadPos * m_iSize]; }
		SQLLEN  getIndicator(const int& iCurrentArrayReadPos) { return m_vecIndicators[iCurrentArrayReadPos]; }

		char*       getArrayCharData()  { return m_ptrArrayCharData; }
		SQLLEN*     getArrayIndicators() { return (SQLLEN*)&m_vecIndicators[0]; }
		const int&  getArraySize()  const     { return m_iArraySize; }
		int         getCurrentInsertedElementCount() { return m_iCurrentArrayInsertPosition; }

		void        reset() { m_iCurrentArrayInsertPosition = 0; }
		const int&   getStringSize() const { return m_iSize; }

private:
	int     m_iArraySize;
	//starts with 0
	int     m_iCurrentArrayInsertPosition;

	char*   m_ptrArrayCharData;

	std::vector<SQLLEN> m_vecIndicators;
	int     m_iSize;
};
#endif // MNSOCISTRING_H_INCLUDED
