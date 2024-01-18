# Priority Queue

C++ 內建的 priority queue

```cpp
#include <iostream>
#include <queue>
using namespace std;

int main() {
    // heapsort
    priority_queue<int> pq1; // max_heap
    pq1.push(30);
    pq1.push(70);
    pq1.push(50);
    while (!pq1.empty()) {
        cout << pq1.top() << ", ";
        pq1.pop();
    }
    cout << endl;
    
    priority_queue <int, vector<int>, greater<int> > pq2; // min_heap
    pq2.push(30);
    pq2.push(70);
    pq2.push(50);
    while (!pq2.empty()) {
        cout << pq2.top() << ", ";
        pq2.pop();
    }
    cout << endl;
    return 0;
}
```

可以簡單用 vector 來實作，且可以使用內建的 `push_heap()` 與 `pop_heap()` 函式來做 heapify。

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

template<typename T>
class PriorityQueue {
public:
    void Push(const T& value) {
        data_.push_back(value);
        push_heap(data_.begin(), data_.end()); // HeapifyUp
    }
    void Push(T&& value) {
        data_.push_back(move(value));
        push_heap(data_.begin(), data_.end());  // HeapifyUp
    }
    bool Empty() const {
        return data_.empty();
    }
    const T& Top() const {
        return data_[0];
    }
    void Pop() {
        pop_heap(data_.begin(), data_.end());
        // 將 root 拿掉
        // 把最後一個 leaf 放到 root 位置後做 HeapifyDown
        // 再把原本的 root 放到原本最後一個 leaf 的位置
        data_.pop_back();  // 需要手動把最後一個 leaf (原本的root)拿掉
    }
private:
    vector<T> data_;
};

int main() {
    PriorityQueue<int> pq;
    pq.Push(30);
    pq.Push(70);
    pq.Push(50);
    while (!pq.Empty()) {
        cout << pq.Top() << ", ";
        pq.Pop();
    }
    cout << endl;
    return 0;
}
```