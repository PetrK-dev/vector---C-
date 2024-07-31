#include <cstdlib>

namespace epc
{
   template <typename T, size_t N>
   class vector
   {
       T* m_data;
       size_t m_size;
       size_t m_capacity;
       alignas(T) unsigned char m_buffer[N * sizeof(T)];

   public:
       vector() noexcept : m_size(0), m_capacity(N) {
           m_data = reinterpret_cast<T*>(m_buffer);
       }

       vector(const vector& v) {
           m_capacity = v.m_capacity;

           if (N < m_capacity) {
               m_data = (T*)::operator new(m_capacity * sizeof(T));
           }
           else
               m_data = reinterpret_cast<T*>(m_buffer);
           try {
               for (m_size = 0; m_size < v.m_size; m_size++)
                   new (m_data + m_size) T(std::move(*(v.m_data + m_size)));
           }
           catch (...) {
               clear();
               if(!is_short())
                   ::operator delete(m_data);
               throw;
           }
       }

       vector& operator=(const vector& v) {
           if (this == &v)
               return *this;

           vector tmp(v);
           swap(tmp);
           return *this;
       }

       vector(vector&& v) : m_capacity(N), m_size(0) {
          set_short();
          swap(v);
       }

       vector& operator=(vector&& v) {
           if (this == &v)
              return *this;
         set_short();
         clear();
         swap(v);
         return *this;
       }

       ~vector() {
           clear();
           if (!is_short())
               ::operator delete(m_data);
       }

       T* data() { return m_data; }
       const T* data() const { return m_data; }

       T& operator[](size_t v) { return *(m_data + v); }
       const T& operator[](size_t v) const { return *(m_data + v); }

       size_t capacity() const { return m_capacity; }
       size_t size() const { return m_size; }

       void reserve(size_t new_capacity) {
           if (new_capacity <= m_capacity)
               return;

           T* data = (T*)::operator new(new_capacity * sizeof(T));
           size_t new_size = 0;
           try {
               for (; new_size < m_size; new_size++)
                   new (data + new_size) T(std::move(*(m_data + new_size)));
           }
           catch (...)
           {
               for (; new_size > 0; new_size--)
                   ( data + new_size - 1)->~T();
               ::operator delete(data);
               throw;
           }
           clear();
           if (!is_short())
               ::operator delete(m_data);

           m_data = data;
           m_capacity = new_capacity;
           m_size = new_size;
       }

       void push_back(const T& v) {
           if (m_size >= m_capacity)
               reserve(m_capacity ? m_capacity * 2 : 1);

           new (m_data + m_size) T(std::move(v));
           m_size++;
       }

       void push_back(T&& v) {
           emplace_back(std::move(v));
       }

       template <typename... Ts>
       void emplace_back(Ts&&...args) {
           if (m_size == m_capacity)
               reserve(m_capacity ? 2 * m_capacity : 1);
           new (m_data + m_size) T(std::forward<Ts>(args)...);
           m_size++;
       }

       void pop_back() {
           (m_data + m_size - 1)->~T();
           m_size--;
       }

       void clear() {
           for (; m_size > 0; m_size--)
               (m_data + m_size - 1)->~T();
       }

       void swap(vector& v) {
           if (this->is_short()) {
               if (v.is_short()) {
                   size_t tmp_capacity = v.m_capacity;
                   size_t tmp_size = v.m_size;
                   unsigned char tmp_data[N * sizeof(T)];
                   size_t size = 0;
                   try {
                       for (size = 0; size < v.m_size; size++)
                           new (tmp_data + size) T(std::move(*(v.m_data + size)));
                   } catch (...) {
                       //smaze cast uspesne vytvorenych prvku
                       for (; size > 0; size--)
                           ((T*)tmp_data + size - 1)->~T();
                       throw;
                   }
                   v.clear();
                   try {
                       for ( size = 0; size < m_size; size++)
                           new (v.m_data + size) T(std::move(*(m_data + size)));
                   }
                   catch (...) {
                       //smaze cast uspesne vytvorenych prvku
                       for (; size > 0; size--)
                           (v.m_data + size - 1)->~T();
                       //nacte puvodni prvky zpet do v.m_data
                       for (size = 0; size < tmp_size; size++)
                           new (v.m_data + size) T(std::move(*(tmp_data + size)));
                       v.m_capacity = tmp_capacity;
                       v.m_size = tmp_size;
                       //smaze prvky v pomocnem poli tmp_data
                       for (; tmp_size > 0; tmp_size--)
                           ((T*)tmp_data + tmp_size - 1)->~T();
                       throw;
                   }
                   v.m_capacity = this->m_capacity;
                   v.m_size = this->m_size;
                   clear();
                   try {
                       for (size = 0; size < tmp_size; size++)
                           new (m_data + size) T(std::move(*(tmp_data + size)));
                   }
                   catch (...) {
                       //smaze cast uspesne vytvorenych prvku
                       for (; size > 0; size--)
                           (m_data + size - 1)->~T();
                       //nacte puvodni prvky zpet do m_data
                       for (size = 0; size < v.m_size; size++)
                           new (m_data + size) T(std::move(*(v.m_data + size)));
                       this->m_capacity = v.m_capacity;
                       this->m_size = v.m_size;
                       //smaze prvky v poli ve vectoru v
                       v.clear();
                       v.m_capacity = tmp_capacity;
                       v.m_size = tmp_size;
                       //smaze prvky v pomocnem poli tmp_data
                       for (; tmp_size > 0; tmp_size--)
                           ((T*)tmp_data + tmp_size - 1)->~T();

                   }
                   this->m_capacity = tmp_capacity;
                   this->m_size = tmp_size;
                   for (; tmp_size > 0; tmp_size--)
                       ((T*)tmp_data + tmp_size - 1)->~T();
               }
               else {
                   size_t tmp_capacity = v.m_capacity;
                   size_t tmp_size = v.m_size;
                   T* tmp_data = v.m_data;
                   v.m_capacity = this->m_capacity;
                   v.m_size = this->m_size;
                   v.set_short();
                   size_t size = 0;
                   try {
                       for (size = 0; size < m_size; size++)
                           new (v.m_data + size) T(std::move(*(m_data + size)));
                   }
                   catch (...) {
                       for (; size > 0; size--)
                           (v.m_data + size - 1)->~T();
                       v.m_capacity = tmp_capacity;
                       v.m_size = tmp_size;
                       v.m_data = tmp_data;
                       throw;
                   }
                   clear();
                   this->m_capacity = tmp_capacity;
                   this->m_size = tmp_size;
                   this->m_data = tmp_data;
               }
           }
           else {
               if (v.is_short()) {
                   size_t tmp_capacity = this->m_capacity;
                   size_t tmp_size = this->m_size;
                   T* tmp_data = this->m_data;
                   this->m_capacity = v.m_capacity;
                   this->m_size = v.m_size;
                   set_short();
                   size_t size = 0;
                   try {
                       for ( size = 0; size < v.m_size; size++)
                           new (m_data + size) T(std::move(*(v.m_data + size)));
                   }
                   catch (...) {
                       for (; size > 0; size--)
                           (m_data + size - 1)->~T();
                       this->m_capacity = tmp_capacity;
                       this->m_size = tmp_size;
                       this->m_data = tmp_data;
                       throw;
                   }
                   v.clear();
                   v.m_capacity = tmp_capacity;
                   v.m_size = tmp_size;
                   v.m_data = tmp_data;
               }
               else {
                   size_t tmp_capacity = v.m_capacity;
                   size_t tmp_size = v.m_size;
                   T* tmp_data = v.m_data;
                   v.m_capacity = this->m_capacity;
                   v.m_size = this->m_size;
                   v.m_data = this->m_data;
                   this->m_capacity = tmp_capacity;
                   this->m_size = tmp_size;
                   this->m_data = tmp_data;
               }
           }
       }

       bool is_short() { return m_data == reinterpret_cast<T*>(m_buffer); }
       void set_short() { m_data = reinterpret_cast<T*>(m_buffer); }
   };
}