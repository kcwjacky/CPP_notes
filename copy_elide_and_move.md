# Copy Elision & Move

在說明移動語意之前先說明複製省略，兩者概念類似，都是要對物件的傳遞做優化避免複製。

## Copy Elision

試著回答在以下程式碼的 98 行 `Vector<int> r = CreateRange(5, 10);` 中，會發生幾次複製？

```cpp
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
        std::cout << "copy constructor" << std::endl;
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

Vector<int> CreateRange(int start, int end) {
    Vector<int> ret(end - start+1);
    for (size_t i = 0; i < ret.Size(); i++) {
        ret[i] = start + i;
    }
    return ret;
}

int main() {
    Vector<int> r = CreateRange(5, 10);
    for (size_t i = 0; i < r.Size(); i++) {
        std::cout << r[i] << " ";
    }
    std::cout << std::endl;
}
```

首先在 `CreateRange()` 中要將原本的區域變數回傳至呼叫該函式處時會發生一次複製形成一個暫時物件 (rvalue)，而將這個暫時物件複製給左值 `r` 的時候會發生第二次複製。因此我們在複製建構中新增 log，來觀察複製：

### Coliru 預設編譯執行：
> g++ -std=c++17 -O2 -Wall -pedantic -pthread main.cpp && ./a.out
```
5 6 7 8 9 10
```
Coliru 預設編譯執行我們並沒有印出任何複製的 log 出來，這是因為編譯器與 C++17 標準分別對複製行為做了優化，使複製不會發生。

### no-elide-constructors
> g++ -std=c++17 -O2 -Wall -pedantic -fno-elide-constructors -pthread main.cpp && ./a.out
```
copy constructor
5 6 7 8 9 10 
```
首先我們可以使用 no-elide-constructors flag 來指定編譯器 (gcc) 將複製省略關閉 (只有在 C++ 標準中沒有被提及的複製省略會被關閉)。因此在關閉後，以上例子中的區域變數複製至 caller 的複製 `T f() {return T();}` 就會出現。

### C++11
> g++ -std=c++11 -O2 -Wall -pedantic -fno-elide-constructors -pthread main.cpp && ./a.out
```
copy constructor
copy constructor
5 6 7 8 9 10 
```
使用較舊的版本 (C++11) 來呈現 C++17 標準中對上述例子的第二種複製做的優化 (rvalue assign to lvalue)。

[Copy elision 參考文件](https://en.cppreference.com/w/cpp/language/copy_elision)

### Summary
Copy elision 是否發生會和使用的編譯器與 C++ 標準有關 (有些標準會強制省略)，我們不能預設所處環境一定幫你做優化。

## 移動語意 (Move)
1. 移動語意的目的是要手動實現 copy elision，藉由明確的方式避免複製。
2. C++11 後出現了右值參考，其目的是為暫時物件取別名，然後即可拿使用。例如：`int&& a = 3;`。右值參考回收利用暫時物件也稱為移動語意。此移動概念是為了要與複製做出區別。
3. 右值參考衍生出了移動建構 (move constructor) 與移動賦值 (move assignment)
    ```cpp
    Object(Object&&) {
        cout << "move constructor" << endl;
    }
    Object& operator=(Object&&) {
        cout << "move assignment" << endl;
        return *this;
    }
    ```
* rule of three $\rightarrow$ rule of five
* 區別在於 rhs 所給的物件為一般物件或是暫時物件
  * 一般物件會呼叫呼叫版本：`Object obj1 = obj2;`
  * 暫時物件匯出叫移動版本：`Object obj1 = Object();`
* 若無實現移動建構與移動賦值時，則都會用複製建構與複製賦值來實作。


為了 disable 複製建構與複製賦值，我們用 unique_ptr 來做例子，例如：`unique_ptr p = obj1;` 由於 unique_ptr 是沒有複製建構與複製賦值的 (= delete)。所以上述指令就會發生錯誤，因為 `obj1` 並不是暫時物件而是一般物件，只能使用複製賦值。此時我們可以將 `obj1` 硬轉型成右值參考：

```cpp
unique_ptr p = Object();
unique_ptr q = p;  // (X) p 為一般物件無法呼叫移動賦值 

unique_ptr r = (unique_ptr<Object>&&) p;            // C 風格轉型
unique_ptr s = static_cast<unique_ptr<Object>&&> p; // C++ 風格
unique_ptr t = move(p);             // C++ 提供轉型為右值參考的函式
```

### Summary
移動語意的目的是手動實現 copy elision，透過對右值參考實現對暫時物件的有效利用，避免不必要的複製造成效能損失。
