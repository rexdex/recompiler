#pragma once

namespace utils
{

	template <typename T>
	class big_vector
	{
	public:
		big_vector()
		{
			m_numTotalElements = 0;
			m_curPage = new Page();
			m_pages.push_back(m_curPage);
		}

		// get size of the buffer
		inline size_t size() const
		{
			return m_numTotalElements;
		}

		// get element by index
		inline T& AllocAt(const uint64_t index)
		{
			while (index >= m_numTotalElements)
				push_back(T());

			const auto page = GetPageIndex(index);
			const auto offset = GetPageOffset(index);
			return m_pages[page]->m_elements[offset];
		}

		// get element by index
		inline T& operator[](const uint64_t index)
		{
			//DEBUG_CHECK(index < m_numTotalElements);
			const auto page = GetPageIndex(index);
			const auto offset = GetPageOffset(index);
			return m_pages[page]->m_elements[offset];
		}

		// get element by index
		inline const T& operator[](const uint64_t index) const
		{
			//DEBUG_CHECK(index < m_numTotalElements);
			const auto page = GetPageIndex(index);
			const auto offset = GetPageOffset(index);
			return m_pages[page]->m_elements[offset];
		}

		// add single element
		inline void push_back(const T& elem)
		{
			if (m_curPage->full())
			{
				m_curPage = new Page();
				m_pages.push_back(m_curPage);
			}
			m_curPage->m_elements[m_curPage->m_numElements] = elem;
			m_curPage->m_numElements += 1;
			m_numTotalElements += 1;
		}

		// add multiple elements
		void push_back(const T* elems, const uint32_t numElems)
		{
			for (uint32_t i = 0; i < numElems; ++i)
				push_back(elems[i]);
		}

		// export to normal std vector
		void exportToVector(std::vector<T>& outVector) const
		{
			outVector.resize(m_numTotalElements);

			auto* writePtr = (T*)outVector.data();
			for (const auto* page : m_pages)
			{
				memcpy(writePtr, page->m_elements, page->m_numElements * sizeof(T)); // TODO: normal copy
				writePtr += page->m_numElements;
			}
		}

	private:
		static const uint32_t PAGE_SIZE = 1 << 20;
		static const uint32_t ELEMS_PER_PAGE = PAGE_SIZE / sizeof(T);

		struct Page
		{
			T m_elements[ELEMS_PER_PAGE];
			uint32 m_numElements;

			inline const bool full() const
			{
				return (m_numElements == ELEMS_PER_PAGE);
			}

			inline Page()
				: m_numElements(0)
			{}
		};

		std::vector<Page*> m_pages;
		Page* m_curPage;

		uint64_t m_numTotalElements;

		static const uint32_t GetPageIndex(const uint64_t index)
		{
			return (uint32_t)(index / ELEMS_PER_PAGE);
		}

		static const uint32_t GetPageOffset(const uint64_t index)
		{
			return (uint32_t)(index % ELEMS_PER_PAGE);
		}
	};

} // utils