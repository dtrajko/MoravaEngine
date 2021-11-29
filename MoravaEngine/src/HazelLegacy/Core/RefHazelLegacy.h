#pragma once

#include <stdint.h>
#include <utility>


namespace Hazel {

	class RefCountedHazelLegacy
	{
	public:
		void IncRefCount() const
		{
			m_RefCount++;
		}
		void DecRefCount() const
		{
			m_RefCount--;
		}

		uint32_t GetRefCount() const { return m_RefCount; }

	private:
		mutable uint32_t m_RefCount = 0; // TODO: atomic
	};

	template<typename T>
	class RefHL
	{
	public:
		RefHL()
			: m_Instance(nullptr)
		{
		}

		RefHL(void* n)
			: m_Instance(nullptr)
		{
		}

		RefHL(T* instance)
			: m_Instance(instance)
		{
			static_assert(std::is_base_of<RefCountedHazelLegacy, T>::value, "Class is not RefCountedHazelLegacy!");

			IncRef();
		}

		template<typename T2>
		RefHL(const RefHL<T2>& other)
		{
			m_Instance = (T*)other.m_Instance;
			IncRef();
		}

		template<typename T2>
		RefHL(RefHL<T2>&& other)
		{
			m_Instance = (T*)other.m_Instance;
			other.m_Instance = nullptr;
		}

		~RefHL()
		{
			DecRef();
		}

		RefHL(const RefHL<T>& other)
			: m_Instance(other.m_Instance)
		{
			IncRef();
		}

		RefHL& operator=(void*)
		{
			DecRef();
			m_Instance = nullptr;
			return *this;
		}

		RefHL& operator=(const RefHL<T>& other)
		{
			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		RefHL& operator=(const RefHL<T2>& other)
		{
			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		RefHL& operator=(RefHL<T2>&& other)
		{
			DecRef();

			m_Instance = other.m_Instance;
			other.m_Instance = nullptr;
			return *this;
		}

		operator bool() { return m_Instance != nullptr; }
		operator bool() const { return m_Instance != nullptr; }

		T* operator->() { return m_Instance; }
		const T* operator->() const { return m_Instance; }

		T& operator*() { return *m_Instance; }
		const T& operator*() const { return *m_Instance; }

		T* Raw() { return  m_Instance; }
		const T* Raw() const { return  m_Instance; }

		void Reset(T* instance = nullptr)
		{
			DecRef();
			m_Instance = instance;
		}

		template<typename T2>
		RefHL<T2> As() const
		{
			return RefHL<T2>(*this);
		}

		template<typename... Args>
		static RefHL<T> Create(Args&&... args)
		{
			return RefHL<T>(new T(std::forward<Args>(args)...));
		}
	private:
		void IncRef() const
		{
			if (m_Instance)
			{
				m_Instance->IncRefCount();
			}
		}

		void DecRef() const
		{
			if (m_Instance)
			{
				m_Instance->DecRefCount();
				if (m_Instance->GetRefCount() == 0)
				{
					delete m_Instance;
				}
			}
		}

		template<class T2>
		friend class RefHL;
		T* m_Instance;
	};

}
