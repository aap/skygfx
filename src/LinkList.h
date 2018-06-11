#ifndef __LINKLIST__
#define __LINKLIST__

template <class T>
class CLink
{
public:
	inline void Insert(CLink<T>* pAttach) {
		pAttach->m_pNext = m_pNext;
		m_pNext->m_pPrev = pAttach;

		pAttach->m_pPrev = this;
		m_pNext = pAttach;
	}

	inline void Remove(void) {
		m_pNext->m_pPrev = m_pPrev;
		m_pPrev->m_pNext = m_pNext;
	}

	inline T& V(void) {
		return m_pItem;
	}

	T m_pItem; // 0-4
	// an item
	CLink<T>* m_pPrev; // 4-8
	//next link in the list
	CLink<T>* m_pNext; // 8-12
	//prev link in the list
};

template <class T>
class CLinkList {
public:
	CLinkList(void)
		: m_plnLinks(nullptr)
	{
	}

	void Init(int nNumLinks) {
		m_plnLinks = new CLink<T>[nNumLinks];

		m_lnListHead.m_pNext = &m_lnListTail;
		m_lnListTail.m_pPrev = &m_lnListHead;

		m_lnFreeListHead.m_pNext = &m_lnFreeListTail;
		m_lnFreeListTail.m_pPrev = &m_lnFreeListHead;

		for(int i = nNumLinks - 1; i >= 0; i--) {
			m_lnFreeListHead.Insert(&m_plnLinks[i]);
		}
	}

	void Shutdown(void) {
		delete[] m_plnLinks;

		m_plnLinks = nullptr;
	}

	CLink<T>* InsertSorted(const T& pItem) {
		CLink<T>* pLink = m_lnFreeListHead.m_pNext;

		if(pLink == &m_lnFreeListTail) {
			return nullptr;
		}

		pLink->m_pItem = pItem;

		pLink->Remove();

		CLink<T>* pInsertAfter = &m_lnListHead;

		while(pInsertAfter->m_pNext != &m_lnListTail && pInsertAfter->m_pNext->m_pItem < pItem) {
			pInsertAfter = pInsertAfter->m_pNext;
		}

		pInsertAfter->Insert(pLink);

		return pLink;
	}

	CLink<T>* Insert(const T& pItem) {
		CLink<T>* pLink = m_lnFreeListHead.m_pNext;

		if(pLink == &m_lnFreeListTail) {
			return nullptr;
		}

		pLink->m_pItem = pItem;

		pLink->Remove();
		m_lnListHead.Insert(pLink);

		return pLink;
	}

	void Clear(void) {
		while(m_lnListHead.m_pNext != &m_lnListTail) {
			Remove(m_lnListHead.m_pNext);
		}
	}

	void Remove(CLink<T>* pLink) {
		pLink->Remove();
		m_lnFreeListHead.Insert(pLink);
	}

	CLink<T>* Next(CLink<T>* pCurrent) {
		if(pCurrent == 0) {
			pCurrent = &m_lnListHead;
		}

		if(pCurrent->m_pNext == &m_lnListTail) {
			return 0;
		}
		else {
			return pCurrent->m_pNext;
		}
	}

	int CountElements(void) {
		int n = 0;
		for ( auto i = m_lnListTail.m_pPrev; i != &m_lnListHead; i = i->m_pPrev )
			n++;
		return n;
	}

	int CountFreeElements(void) {
		int n = 0;
		for ( auto i = m_lnFreeListTail.m_pPrev; i != &m_lnFreeListHead; i = i->m_pPrev )
			n++;
		return n;
	}


	CLink<T> m_lnListHead; // 0-12
	//head of the list of active links
	CLink<T> m_lnListTail; // 12-24
	//tail of the list of active links
	CLink<T> m_lnFreeListHead; // 24-36
	//head of the list of free links
	CLink<T> m_lnFreeListTail; // 36-48
	//tail of the list of free links
	CLink<T>* m_plnLinks; // 48-52
	//pointer to actual array of links
};

#endif