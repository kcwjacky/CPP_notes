# C++ Initialization

C++ 的初始化非常的複雜，以下說明使用 `()` 與 `{}` 的優缺點。

```cpp
#include <iostream>

class A {
public:
    A() { std::cout << "A constructor without any argument" << std::endl; }
    // A 類別的建構式不用帶參數
};

class B{
public:
    B(int v) { std::cout << "B constructor with an argument " << v << std::endl; }
    // B 類別的建構式需要帶參數
};

A a2() {
    std::cout << "dummy function" << std::endl;
    return A();
}

int main() {
    A a1;  // 最常用的初始化，其實是一種特例，以下說明
    
    B b1(10);  // 產生出 B 類別的物件
    A a2();    // 此為函式宣告
               // 因為為了與 C 相容
               // 所以這裡會解讀為 a2 函數的宣告
               // 而非產生出一個物件
               // 因此出現了在有無參數產生物件的不一致性
    
    A a3{};
    B b2{2};
    // 現代 C++ 會預期用大括弧來初始化
    
    // 精度議題
    int i(3);    // C++ 為了泛用，支援此種寫法
    int j(3.2);  // 3.2 隱性轉型 成 3
    int k{3};   
    // int l{3.2};  // C++ 不希望 narrowing conversion 發生，所以使用 {} 會做精度檢查
    int m{};        // zero initialization 
    
    return 0;
}
```

以上 `a2` 的例子可以對應到 `std::thread t(Hello2());` 這一個指令，同樣會解讀成一個函式 `t` 的宣告。因此若放入 thread 的參數為函式類別物件時，務必要先產生出一個有名字的類別物件如： `Hello2 h2; std::thread t(h2); `