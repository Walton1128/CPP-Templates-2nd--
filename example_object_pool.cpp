#include <iostream>
#include <memory>
#include <queue>
#include <boost/shared_ptr.hpp>
using namespace std;

template<bool COND, typename TrueType, typename FalseType>
struct IfThenElseT {
    using Type = TrueType;
};

template<typename TrueType, typename FalseType>
struct IfThenElseT<false, TrueType, FalseType> {
    using Type = FalseType;
};
template<bool COND, typename TrueType, typename FalseType>
using IfThenElse = typename IfThenElseT<COND, TrueType, FalseType>::Type;


template <typename Head, typename... Tail>
class LargestTypeT
{
    private:
        using First = Head;
        using Rest = typename LargestTypeT<Tail...>::Type;
    public:
        using Type = IfThenElse<(sizeof(First) >= sizeof(Rest)), First, Rest>;
};

template<typename _Type>
class LargestTypeT<_Type>
{
    public:
        using Type = _Type;
};
template<typename... Args>
using LargestType = typename LargestTypeT<Args ...>::Type;


template <typename T>
class ObjectCounter
{
    public:
        static unsigned int allocate_counter;
        static unsigned int object_counter;

        ~ObjectCounter(){
            std::cout << "object_counter =" << object_counter << "  allocated_mem counter " << allocate_counter << std::endl; 
        }
};

template<typename T>
unsigned int ObjectCounter<T>::allocate_counter = 0;

template<typename T>
unsigned int ObjectCounter<T>::object_counter = 0;


template <typename _PacketType, unsigned _PoolSize = 60>
class ObjectMemPool:ObjectCounter<ObjectMemPool<_PacketType>>
{
    public:
        typedef _PacketType* Pointer;

        ObjectMemPool():m_RingStart{0}, m_RingEnd{_PoolSize - 1}{
            for(int i = 0; i< _PoolSize; ++i){
                m_RingBuffer[i] = i;
            }
        }

        ObjectMemPool(const ObjectMemPool& ) = delete;

        template <typename _Type, typename ... Args>
            typename _Type::Pointer  Get(Args&&... args){
                void* __p;
                int index = 0;
                if(m_RingStart == m_RingEnd){
                    ++(this->allocate_counter);
                    ++(this->object_counter);
                    return typename _Type::Pointer(::new _Type(std::forward<Args>(args)...));
                }else{
                    index = m_RingBuffer[m_RingStart]; 
                    m_RingStart = (m_RingStart+1) % _PoolSize;
                    __p = (void*)(m_Pool + index);//m_MemQueue.front();
                    ::new(__p) _Type(std::forward<Args>(args)...);

                    ++(this->object_counter);
                    return  typename _Type::Pointer((_Type*)__p, [this, index]( _Type* p){
                            p->~_Type();
                            this->m_RingEnd = (this->m_RingEnd + 1) % _PoolSize;
                            this->m_RingBuffer[this->m_RingEnd] = index;
                            }
                            );
                }
            }

        ~ObjectMemPool(){
            ::operator delete(m_Pool);
        }


    private:
        int m_RingBuffer[_PoolSize]; 
        int m_RingStart;
        int m_RingEnd;
        Pointer m_Pool = static_cast<Pointer> (::operator new(_PoolSize*sizeof(_PacketType)));
};


struct T1{
    int dummy_1;
    int dummy_2;
    using Pointer = std::shared_ptr<T1>;

    T1(int _d1, int _d2):dummy_1(_d1), dummy_2(_d2){;}
    T1() = default;
};


struct T2{
    int dummy_1;
    int dummy_2;
    int dummy_3;
    using Pointer = boost::shared_ptr<T2>;

    T2(int _d1, int _d2, int _d3):dummy_1(_d1), dummy_2(_d2), dummy_3(_d3){;}
    T2() = default;
};

int main(){
    using MaxType = LargestType<T1, T2>;
    ObjectMemPool<MaxType, 3> pmp;

    {
        auto t1 = pmp.Get<T1>();
        auto t1_a = pmp.Get<T1>(100, 100);
        auto t2 = pmp.Get<T2>();
        auto t2_a = pmp.Get<T2>(100, 100, 100);
    }

    auto t1 = pmp.Get<T1>();
    auto t1_a = pmp.Get<T1>(100, 100);
    auto t2 = pmp.Get<T2>();
    auto t2_a = pmp.Get<T2>(100, 100, 100);

    return 0;
}

