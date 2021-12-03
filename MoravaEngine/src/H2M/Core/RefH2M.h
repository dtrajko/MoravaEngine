/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <stdint.h>
#include <utility>


namespace H2M {

	class RefCountedH2M
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
	class RefH2M
	{
	public:
		RefH2M()
			: m_Instance(nullptr)
		{
		}

		RefH2M(void* n)
			: m_Instance(nullptr)
		{
		}

		RefH2M(T* instance)
			: m_Instance(instance)
		{
			static_assert(std::is_base_of<RefCountedH2M, T>::value, "Class is not RefCountedH2M!");

			IncRef();
		}

		template<typename T2>
		RefH2M(const RefH2M<T2>& other)
		{
			m_Instance = (T*)other.m_Instance;
			IncRef();
		}

		template<typename T2>
		RefH2M(RefH2M<T2>&& other)
		{
			m_Instance = (T*)other.m_Instance;
			other.m_Instance = nullptr;
		}

		~RefH2M()
		{
			DecRef();
		}

		RefH2M(const RefH2M<T>& other)
			: m_Instance(other.m_Instance)
		{
			IncRef();
		}

		RefH2M& operator=(void*)
		{
			DecRef();
			m_Instance = nullptr;
			return *this;
		}

		RefH2M& operator=(const RefH2M<T>& other)
		{
			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		RefH2M& operator=(const RefH2M<T2>& other)
		{
			other.IncRef();
			DecRef();

			m_Instance = other.m_Instance;
			return *this;
		}

		template<typename T2>
		RefH2M& operator=(RefH2M<T2>&& other)
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
		RefH2M<T2> As() const
		{
			return RefH2M<T2>(*this);
		}

		template<typename... Args>
		static RefH2M<T> Create(Args&&... args)
		{
			return RefH2M<T>(new T(std::forward<Args>(args)...));
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
		friend class RefH2M;
		T* m_Instance;
	};

}
