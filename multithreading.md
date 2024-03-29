 # Multithreading in C++
## Basic
### 執行緒的啟動與等待結束

* 使用 std::thread 產生 thread object
* 在建構時傳入一個 callable 型態即可啟動一個執行緒，其中 callable 為: 
  1. 函式指標 
  2. 函式類別物件
  3. 匿名函式

```cpp
#include <iostream>
#include <thread>
#include <chrono>

static void Hello1() {
    std::cout << "Hello 1 Concurrent World!" << std::endl;
}

class Hello2 {
public:
    void operator()() {
        std::cout << "Hello 2 Concurrent World!" << std::endl;
    }
};

int main() {    
    // 1. 傳入函式指標
    std::thread t1(Hello1);
    
    // 2. 函式類別物件
    Hello2 h2;  // h2 為類別物件，支援 h() 
    std::thread t2(h2); 
    // std::thread t2(Hello2()); 
    // 無法直接用類別加上小括弧生出類別物件傳入
    // 因為編譯器會把 t2 解讀為一個回傳 std::thread ，引數為一個沒有名字的函式
    // 該無沒有名字的函式 Hello2 ___ () 為回傳 Hello2 的物件且不用吃參數的函數
    // std::thread t2{Hello2()};  <-- 用 uniform initialization 可以避開此問題
    
    
    // 3. 匿名函式
    std::thread t3([](){std::cout << "Hello 3 Concurrent World!" << std::endl;});
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // sleep 100 ms
    
    t1.join();
    t2.join();
    t3.join();
    // t3.detach();  // 背景執行
    
    return 0;
}
```

### 啟動執行緒時傳入資料/參考

```cpp
#include <iostream>
#include <thread>

static void Hello(int i, std::string name, int& val) {
    std::cout << "Hello Concurrent World! " << i << " "<< name << std::endl;
    val = 0;
}

int main() {
    int val = 10;
    
    std::thread t1(Hello, 1, "Jacky", std::ref(val));
    // 1. 將要傳入的資料直接放到引數中即可將資料傳入執行緒
    // 2. 在啟動執行緒時傳入資料時，預設行為都是用複製
    //    所以無法直接傳參考，編譯會失敗
    //    因此使用 std::ref 傳入物件的指標後就可隱性轉型成參考
    
    t1.join();
    std::cout << val << std::endl;
    return 0;
}
```

### 轉移執行緒物件的擁有權

```cpp
#include <iostream>
#include <thread>

// RAII
class ScopedThread {
public:
    ScopedThread(std::thread&& t) : t_(std::move(t)) {}
    ~ScopedThread() {
        if (t_.joinable())
            t_.join();
    }
private:
    std::thread t_;
};

ScopedThread CreateThread() {
    // 函式回傳會觸發移動語 (move sematic)，同時也可能發生複製省略 (copy elision)
    return ScopedThread(std::thread([](){ std::cout << "Hello, Concurrent World!" << std::endl; }));
}

int main() {
    std::thread t1{[](){ std::cout << "Hello, Concurrent World!" << std::endl; }};
    // std::thread t2 = t1;
    // std::thread 物件不能複製，因為其複製建構子 = delete
    std::thread t2 = std::move(t1); // 但可以藉由移動轉移擁有權
    t2.join();
    
    ScopedThread t3 = CreateThread(); // RAII 無須手動 join
    return 0;
}
```
## 在多執行緒間共享資料
### Race Condition
執行結果與不同執行緒間執行的先後順序有關，當結果不符合預期的時候稱之。

#### Mutex
基本用法
```cpp
std::mutex m;
m.lock();  // busy waiting
m.unlock();
```

#### RAII
`std::lock_guard l(m)` 使用 `lock_guard` 類別的物件，並傳入要管理的 `std::mutex` 其建構時候就會鎖上，並且在解構時候自動解鎖。與 lock_guard 對應的是 `std::unique_lock l(m)`，其允許手動解鎖，而不限定只有在解構的時候解鎖。基於 C++ Zero-overhead principle，若不需要提早 unlock 用 lock_guard 即可，因為 unique_lock 功能較複雜所以必定會有額外的 overhead。

#### Lambda Function
`[ captures ] ( params ) { body }` <br>
captures 中放入等號 `[=]` 表示匿名函式內使用到的外部自動變數預設會在匿名函式呼叫時複製進來，而 `[&]` 則表示變數預設使用參考傳入。也可以混著一起用如 `[=, &m]` 表示匿名函式中用到的變數預設是從外層 scope 中複製進去的，除了 `m` 這個變數是要用參考型態傳入。或是 `[&, k]` 表示所有的變數都是參考，除了 `k` 是複製一份進去不共用。

```cpp
int Sum(int n) {
    std::mutex m;
    size_t numberOfRanges = std::thread::hardware_concurrency();
    // 查詢目前硬體最多支援多少個同時執行的執行緒
    // 但不一定支援，應該要做檢查 (無法獲取該資訊時會回傳 0)。

    int sizeOfRange = (n - 1) / numberOfRanges + 1;
    int result = 0;
    std::vector<std::thread> threads(numberOfRanges);
    for (size_t k = 0; k < numberOfRanges; k++) {
        threads[k] = std::thread(
            [=, &m, &result]() {
                int begin = k * sizeOfRange + 1;
                int end = std::min((int)(k + 1) * sizeOfRange, n);
                int s = 0;
                for (int i = begin; i <= end; i++) {
                    s += i;
                }
                std::lock_guard l(m);
                result += s;
            });
    }
    for (auto& t : threads) {
        t.join();
    }
    return result;
}

int main() {
    std::cout << Sum(100) << std::endl;
}
```

#### mutable

類別函式在不會修改到物件內容時會在函式末加上 const 修飾，但當物件內容包含 mutex 時候，在上鎖解鎖時候就會違背 const 的含義，因此在 mutex 宣告時可以加上 `mutable`關鍵字表示該 mutex 為特例，不在 const 規範之中。C++ 所提供的資料結構大部分都不是 thread-safe 的，以下為自己實作的 thread-safe stack 來說明 mutable 關鍵字：

```cpp
class ThreadSafeStack {
    std::vector<int> impl_;
    mutable std::mutex m_;
    // 每個物件擁有各自的 mutex
    // mutable 修飾字的目的是用於唯讀的方法中
    // 例如以下的 int Top() const
    // 由於使用 lock_guard 所以還是會動到 mutex m_ 的狀態
    // 加上 mutable 有類似法外開恩的用途
public:
    ThreadSafeStack() = default;
    ThreadSafeStack(const ThreadSafeStack&) = delete;
    void Push(int v) {
        std::lock_guard l(m_);
        impl_.push_back(v);
    }
    bool Pop(int& v) {
        std::lock_guard l(m_);
        if (impl_.empty()) {
            return false;
        }
        v = impl_.back();
        impl_.pop_back();
        return true;
    }
    int Top() const {
        std::lock_guard l(m_);
        return impl_.back();
    }
    bool Empty() const {
        std::lock_guard l(m_);
        return impl_.empty();
    }
};
```

### Deadlock

#### Criteria
* Mutual Exclusion
* Hold and Wait
* No Preemption
* Circular Wait

為了避免死結的發生，最直覺的方法就是用單一個 mutex 來形成 critical section，確保同一時間只有一個執行緒能夠存取該程式碼區中的資源。在實務上，我們可能同時有好多資源是需要被保護的，所以比較簡單的方法是對於每一個資源都有各自對應的 mutex 來保護。但如同哲學家晚餐的問題一樣，對於多個互斥鎖，我們必須要有一個固定的上鎖順序來避免 deadlock 的發生。C++ 同樣支援此功能，我們可以在 `std::lock()` 當中放入多個 mutex (如：`std::lock(ma, mb);`)，來確保一個固定加鎖順序，對參數中的所有 mutex 判斷大小（或排序）來做上鎖順序的依據。

```cpp
int a = 1, b = 2;
std::mutex ma, mb;
std:: thread([&](){
    std::lock(ma, mb);  // 確保一個固定的上鎖順序
    std::lock_guard la(ma, std::adopt_lock);
    std::lock_guard lb(mb, std::adopt_lock);
    // 不能忘記解鎖，所以使用 lock_guard 來幫忙解鎖
    // 但由於不能重複上鎖，所以傳入 std::apopt_lock
    int t = a;
    a = b;
    b = t;
});
```
以上太麻煩了，C++ 有內建的 `scoped_lock()`
```cpp
int a = 1, b = 2;
std::mutex ma, mb;
std:: thread([&](){
    std::scoped_lock l(ma, mb);
    // 整合了 std::lock 與 std::lock_guard
    int t = a;
    a = b;
    b = t;
});
```

### once_flag & call_once

`std::once_flag` 與 `std::call_once` 是 C++11 引入用來確保在多執行緒中，該函式只會被呼叫一次。常用於避免每一個執行緒都會做重複的初始化。

```cpp
int result;
std::once_flag initialized;
std::vector<std::thread> threads;
for (int k = 0; k < 10; ++k) {
    threads.push_back(
        std::thread([&, k]() {
            std::call_once(initialized, [&]() { 
                result = 1; });                
            std::cout << result;
        }));
}
for (auto& t : threads) t.join();
```

### 鎖的讀寫分級
* 沒有限制
* 獨佔模式－上鎖後只有該執行緒可以執行該 critical section。
  * std::lock_guard (無法自己解鎖) 
  * std::unique_lock（可以自己解鎖）
* 唯獨模式－允許同時很多個執行緒執行該 critical section，但是只能對資料做讀取而不能寫入
  * std::shared_lock
  
```cpp
class ThreadSafeVector {
    std::vector<int> v_;
    mutable std::shared_mutex m_;
public:
    ThreadSafeVector() = default;
    ThreadSafeVector(const ThreadSafeVector&) = delete;
    void Search(int target) const {
        std::shared_lock l(m_); 
        // 唯獨模式下運行，多個執行緒可以同時查找 vector 內容
        std::string r = "Search " + std::to_string(target) + " from ";
        for (int x : v_) {
            r += std::to_string(x) + " ";
        }
        for (int x : v_) {
            if (x == target) {
                r += "(Found)";
                break;
            }
        } 
        r += "\n";
        std::cout << r;
    }    
    void PushBack(int value) {
        std::lock_guard l(m_); 
        // 獨佔模式下運行，只有一個 thread 上鎖時，才能進入
        v_.push_back(value);
    }    
};
```

### 可遞迴的互斥鎖

`std::recursive_mutex` 是 C++ 標準庫中提供的一種互斥量（mutex）類型，它允許同一執行緒多次對一個 mutex 做上鎖，而不會導致死鎖。這使得它在一些特定的場景中非常有用，尤其是在遞迴函數調用中。筆記而已，應該使用場景較少。

## Synchronization

### 等待事件發生或條件成立

* `std::lock` 其實是 busy waiting，所以為了可以把 CPU 的執行權讓給其他的執行緒，我們需要不停的 lock 與 unlock 
    ```cpp
    m.lock();
    while (flag) {
        m.unlock();
        m.lock();
    }
    ```
* 在 unlock 與 lock 之間睡一下，避免過於 busy。但這樣不夠準確，可能會睡過頭，導致資料可能已改變了但還在睡。
* 可以使用 C++ 標準庫中的 `std::condition_variable` 與 `std::condition_variable_any`，等待條件成立時自動會通知在睡覺的 mutex。
    ```cpp
    // 一個 thread-safe 的 queue
    // 當有人對其 enqueue 時，有另外一個 thread 會被叫醒來做 dequeue

    ThreadSafeQueue() {
        std::mutex m_;
        std::condition_variable cond_;  // 1. 宣告
        std::deque<int> v_;
    }

    void waitAndDequeue(int& value) {
        std::unique_lock l(m_);
        cond_.wait(l, [this]() {return !v_.empty();});
        // 2. 這邊會判斷條件成立與否，當不成立時，就會解鎖然後去睡覺等待通知
        //    因為涉及到解鎖，所以上面必須使用 unique_lock 而非只在解構時解鎖的 lock_guard
        // 4. 被叫醒後會嘗試搶鎖，搶到後就會往下做事
        value = v_front();
        v_pop_front();
    }
    
    void equeue(int value) {
        std::lock_guard l(m_);
        v_.push_back(value);
        cond_.notify_one();  
        // 3. 在準備 unlock 的時候，去通知/叫醒在等這個鎖的執行緒來「排隊」
    }
    ```

### 取得非同步函式的回傳值

```cpp
// 用基本的語法來實作取得執行緒中所處理的值
void getValue(int &value) {
    value = 1;
}

int main() {
    int value = 0;
    std::thread t(GetValue, std::ref(value));
    t.join();
    std::cout << value << std::endl;
}
```

* 舊的寫法很麻煩的，且無法取得回傳值，這在 functional programming 會有問題。我們會希望上述的 `getValue` 單純的當作一個 function 來使用，並且取得它的回傳值。因此我們可以使用 `std::async` 這個函式，其回傳值的型態為 `std::future<T>`，而參數為要跑的函式。
    ```cpp
    // 使用 std::async 與 std::future<T>
    int getValue() {
        return 1;
    }

    int main() {
        std::future<int> f = std::async(GetValue);
        std::cout << f.get() << std::endl;
    }
    ```
* `std::future<T>` 有兩種策略：
  * 會產生 thread 來跑指定的函式
  * 不會產生 thread，直到呼叫 `std::future<T>::get` 的時候，才會去執行指定函式
  * 預設策略是實作定義，大部分是會開 thread，設定可用 `std::launch::async` / `std::launch::deferred`
* `std::async` 事實上是比較高階的實作，比較底層的實作可以用 `std::packaged_task` 包一個函式呼叫來實作
    ```cpp
    // 使用 std::packaged_task 來實作以上 std::launch::async 策略
    int main() {
        std::packaged_task<int()> task(GetValue);
        std::future<int> f = task.get_future();
        std::thread(std::move(task)).detach(); // 開背景執行 thread
        std::cout << f.get() << std::endl;     // 反正 get 會等，如果背景還沒跑完時
    }
    ```
    ```cpp
    // 使用 std::packaged_task 來實作以上 std::launch::deferred 策略
    int main() {
        std::packaged_task<int()> task(GetValue);
        std::future<int> f = task.get_future();
        task(); // 直接執行
        std::cout << f.get() << std::endl;
    }
    ```

### 將寫入與讀取資料的物件分開

上一段利用 `std::async` 與 `std::packaged_task` 來呼叫一個非同步函式並取回回傳值。另外一個同步機制是類似 pipe 一樣，使用 `std::promise<T>` 產生物件用來寫入資料；使用 `std::future` 產生物件來 `get` 用來讀取資料。這是 C++ CSP (Communicating sequential processes) 的基礎。

```cpp
void getValue(std::promise<int> p) {
    p.set_value(1);
}

int main() {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::thread(std::move(task)).detach();
    std::cout << f.get() << std::endl;
}
```

`std::promise<T>` 更神奇的地方是可以拿來傳 exception。

```cpp
std::promise<int> p;
std::future<int> f = p.get_future();
try {
    throw std::runtime_error("Error");
} catch (const std::exception& e) {
    p.set_exception(std::current_exception());
}
std::cout << "Continue" << std::endl;
std::cout << f.get() << std::endl;


$ g++ -std=c++20 -O2 -Wall -pedantic -pthread main.cpp && ./a.out
Continue
terminate called after throwing an instance of 'std::runtime_error'
  what():  Error
bash: line 7:   612 Aborted                 (core dumped) ./a.out
```

多個執行緒等待同一個資料

```cpp
void Run(std::shared_future<int> f, int i) {
    int target = f.get();
    if (i == target) {
        std::cout << "Run: " << i << std::endl;
    }
}

int main() {
    std::vector<std::thread> threads;
    std::promise<int> p;
    std::shared_future<int> f = p.get_future();
    for (int k = 0; k < 10; k++) {
        threads.push_back(std::thread(Run, f, k));
    }
    std::cout << "Prepare" << std::endl;
    p.set_value(5);
    for (auto& t : threads) t.join();
}
```

## C++ 的記憶體模型

以上都是 C++ 多執行緒的基本操作，接下來是另外一個世界...

### 記憶體的同步

* 一個程式可以包含一個或多個執行緒 (thread of execution)
* 不同執行緒如果存取同樣記憶體位址的物件，在沒有進行同步 (synchronized-with) 的狀況下，是未定義行為 (undefined behavior)
* 可以使用 std::mutex 或 std::atomic<T> 來進行同步

用前面 `std::call_once` 的例子來說明記憶體之間的同步

```cpp
// 有 10 個 threads，但只要一個 thread 替 result 做初始化
int main() {
    int result;
    bool initialized = false;
    std::mutex m;
    std::vector<std::thread> threads;
    for (int k = 0; k < 10; ++k) {
        threads.push_back(
            std::thread([&, k]() {
                if (!initialized) {
                    std::lock_guard l(m);
                    if (!initialized) {
                        // 雙重檢查鎖 (double-checked locking) 
                        // 來防止 initialized 變數發生 race condition
                        result = 0;
                        initialized = true;
                    }
                }
                std::cout << result;
            }));
    }
    for (auto& t : threads) t.join();
    std::cout << result << std::endl;
}
```

以上的程式碼會有問題，因為 `result` 與 `initialized` 在不同的執行緒是沒有進行同步的。例如：threads[0] 執行完 `initialized = true;` 的同時 threads[1] 執行到 `if (!initialized)`，其中 thread[1] 此時的 `initialized` 可能還是 `false`，因為尚未被同步 (各別還在用各自 cache 內的值)。此時我們可以使用 `lock` 與 `unlock` 把 `result` 與 `initialized` 包起來，其會替我們自動完成記憶體的同步，因為 `unlock` 會將使用到的資源更新到主記憶體上，`lock` 則是會從主記憶體去下載最新的資源下來，像是 C 語言當中的 `volatile`。或是直接使用 `std::call_once` 而不要自己寫同步。

以下為 C++ 記憶體同步所需要的功能：
* acquire
  * 用來取得最新記憶體的更新
  * 在 acquire 之後，其它執行緒替共享的資源做的更新，C++ 並不保證你會看到。
  * `mutex.lock();` 的實作中有替我們做了 acquire
* release
  * 用來發布到目前為止的記憶體更新
  * release 之後，其它執行緒也不一定會自動取得發布的最新狀態，其它執行緒必須要執行 acquire 才能取得
  * `mutex.unlock();` 的實作中有替我們做了 release
* relaxed
  * release 會發布所有與其他 threads 所共享物件的記憶體狀態，這樣的做法有時會缺乏效率。所以用 relaxed 可以只同步一個物件記憶體狀態

### Memory order

memory_order 為一個列舉類別用於標示原子操作中記憶體存取的一些重要特性。

順序一致性：sequentially consistent，保證所有執行緒修改的順序都一樣

``` cpp
typedef enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;
```

超難！待整理，好像反正不會用到 XD

## Atomic operation
* atomic 表示 thread-safe
* atomic 的物件無法複製也無法移動，只能參考
* 只有 atomic_flag 一定是無鎖的 (lock-free)，其它如 `template<class T> class atomic<T>` 在操作時，內部可能還是有 lock/unlock 的動作

```cpp
// 用 atomic 練習實作 lock
class Mutex {
    std::atomic<bool> flag_{false};
public:
    void Lock() {
        /*
        while (flag_.load() == true);
        flag_.store(true);
        這寫法會有 race condition，兩行指令中間可能會有其他執行緒見縫插針
        解決方法為使用一個指令來完成兩個指令的工作
        例如 old_val = flag_.exchange(new_val);
        */
        while (flag_.exchange(true));
        flag_.store(true);
    }
    void Unlock() {
        flag_.store(false);
    }
}
```

## Quicksort

### 基本版本
```cpp
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <future>

void QuickSort(std::vector<unsigned int>& nums, size_t start, size_t stop) {
    if (start == stop) return;
    size_t j = start + 1;
    for (size_t i = start + 1; i < stop; i++) {
        if (nums[i] < nums[start]) {
            std::swap(nums[i], nums[j]);
            j++;
        }
    }
    size_t mid = j - 1;
    std::swap(nums[start], nums[mid]);
    
    QuickSort(nums, start, mid);
    QuickSort(nums, mid + 1, stop);
}

void QuickSort(std::vector<unsigned int>& nums) {
    QuickSort(nums, 0, nums.size());
}

int main() {
    std::mt19937 mt{0};
    std::vector<unsigned int> values;
    for (size_t i = 0; i < 100000; ++i) {
        values.push_back(mt());
    }
    auto start = std::chrono::steady_clock::now();
    QuickSort(values);
    auto end = std::chrono::steady_clock::now();
    std::cout << (end - start) << std::endl;
}
```

### 並行版本 
設計並實作一個執行緒安全的佇列，將工作分成多個固定份量的任務，並放到佇列中等待執行，每個執行緒在完成一個任務後會自動繼續從佇列中提取下一個任務來執行，直到沒有任何任務為止。(固定模式使用 producer 與 consumer)

```cpp
#include <vector>
#include <deque>
#include <iostream>
#include <thread>
#include <random>
#include <condition_variable>
#include <chrono>

template<typename T>
class Queue {
    std::deque<T> d_;
    std::mutex m_;
    size_t remaining_{0};
    std::atomic<bool> is_closed_{false};
    std::condition_variable cond_;
public:
    void Enqueue(T value) { 
        std::lock_guard lk(m_);
        remaining_++;
        d_.push_back(value);
        cond_.notify_one();
    }
    void Done() {
        std::lock_guard lk(m_);
        remaining_--;
        if (remaining_ == 0) {
            cond_.notify_all();
        }
    }
    void Close() {
        is_closed_ = true;
        cond_.notify_all();
    }
    bool WaitAndDequeue(T& value) { 
        while (true) {
            std::unique_lock lk(m_);
            cond_.wait(lk, [&]() { return !d_.empty() || (remaining_ == 0 && is_closed_); });
            if (remaining_ == 0 && is_closed_) return false;
            value = d_.front();
            d_.pop_front();
            return true;
        }
    }
};

struct Job {
    size_t start;
    size_t stop;
};

void QuickSort(Queue<Job>& jobs, std::vector<unsigned int>& nums, 
               size_t start, size_t stop) {
    if (start == stop) return;
    size_t j = start + 1;
    for (size_t i = start + 1; i < stop; i++) {
        if (nums[i] < nums[start]) {
            std::swap(nums[i], nums[j]);
            j++;
        }
    }
    size_t mid = j - 1;
    std::swap(nums[start], nums[mid]);
    
    if (mid - start > 10000) {
        jobs.Enqueue({start, mid});         // 當任務有一定負擔時才塞到 queue
    } else {
        QuickSort(jobs, nums, start, mid);  // 反之，直接做掉反而省掉 overhead
    }
    QuickSort(jobs, nums, mid + 1, stop);   // 若同時有兩個任務時，只需要一個塞到 queue 就好
                                            // 當前執行緒本身就可以負責一個任務
}

int main() {
    std::mt19937 mt{0};
    std::vector<unsigned int> values;
    for (size_t i = 0; i < 100000000; ++i) {
        values.push_back(mt());
    }
    Queue<Job> jobs;
    auto start = std::chrono::steady_clock::now();
    {
        int number_of_workers = 8;
        std::vector<std::jthread> workers(number_of_workers);
                
        for (int w = 0; w < number_of_workers; ++w) {
            workers[w] = std::jthread([&jobs, &values]() {
                Job job;
                while (jobs.WaitAndDequeue(job)) {
                    QuickSort(jobs, values, job.start, job.stop);
                    jobs.Done();
                }
            });
        }

        jobs.Enqueue({0, values.size()});
        jobs.Close();
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << (end - start) << std::endl;
    
    for (size_t i = 0; i + 1 < values.size(); ++i) {
        if (values[i] > values[i + 1]) {
            std::cout << "ERROR" << std::endl;
            break;
        }
    }
}
```