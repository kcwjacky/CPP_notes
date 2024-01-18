#include <iostream>

template<typename T>
class Vector {
public:
    Vector() {
        /* default constructor */
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }
    
    Vector(int size) {
        data_ = new T[size]{};
        size_ = size;
        capacity_ = size;
    }
    
    Vector(const Vector<T>& rhs) {
        /* copy constructor */
        size_ = rhs.size_;
        capacity_ = size_;
        data_ = new T[size_]{};
        for (size_t i = 0; i < size_; i++)
            data_[i] = rhs.data_[i];
    }
    
    Vector<T>& operator=(const Vector<T>& rhs) {
        /* copy assignment */
        if (&rhs == this) 
            return *this;
        
        delete[] data_; 
        size_ = rhs.size_;
        capacity_ = size_;
        data_ = new int[size_]{};
        for (size_t i = 0; i < size_; i++)
            data_[i] = rhs.data_[i];
        return *this;
    }
    
    ~Vector() {
        /* destructor */
        delete[] data_;  
    }
    
    const T& operator[](int i) const {
        return data_[i];
    }
    
    T& operator[](int i) {
        return data_[i];
    }
    
    size_t Size() const {
        return size_;
    }
    
    size_t Capacity() const {
        return capacity_;
    }
    
    void PushBack(T elem) {
        if (capacity_ <= size_) {
            capacity_ = capacity_ * 2 + 1;
            T *newData = new T[capacity_];
            for (size_t i = 0; i < size_; i++)
                newData[i] = data_[i];
            delete[] data_;
            data_ = newData;
        }
        data_[size_] = elem;
        ++size_;
        return;
    }
    
private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

int main()
{
    Vector<int> v;
    for (int i = 0; i < 5; i++) {
        v.PushBack(i);
    }
    
    Vector<int> n = v;
    v[0] = 6;
    for (size_t i = 0; i < n.Size(); i++) {
        std::cout << n[i] << " ";
    }
    std::cout << std::endl;
    n = v;
    for (size_t i = 0; i < n.Size(); i++) {
        std::cout << n[i] << " ";
    }
    std::cout << std::endl;
}