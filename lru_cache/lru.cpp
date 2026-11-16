// Question: Design a fixed-capacity cache of integer keys and values that evicts the least recently used key when it is full.
#include <bits/stdc++.h>
using namespace std;

class CacheInterface {
public:
    virtual ~CacheInterface() = default;
    // get the value of key from the cache
    virtual int get(int key) = 0;
    // add or modify the value of key in the cache
    virtual void put(int key, int value) = 0;
    // get the current occupied size of the cache
    virtual size_t size() const = 0;
    // get the maximum capacity of the cache
    virtual size_t capacity() const = 0;
};

/*
 * LRUCache Implementation
 * 
 * Algorithm:
 * 1. Uses a Doubly Linked List (DLL) to maintain the order of elements based on usage.
 *    - The head of the list represents the Most Recently Used (MRU) element.
 *    - The tail of the list represents the Least Recently Used (LRU) element.
 *    - Dummy head and tail nodes are used as sentinels to simplify list operations.
 * 2. Uses an Unordered Map (Hash Map) to store key-value pairs and pointers to corresponding nodes in the DLL.
 *    - This allows O(1) access to any element.
 * 3. Thread Safety:
 *    - A global mutex (mutex_) ensures exclusive access to the cache for public operations (get, put).
 *    - Node-level mutexes are used for fine-grained locking during node manipulation (RemoveNode, AddToFront).
 *    - Detached nodes point to dummy_node_1/2 to ensure pointer safety.
 */
class LRUCache : public CacheInterface {
    size_t capacity_;
    size_t cur_capacity_;
    mutable std::mutex mutex_;
    struct Node {
        int key;
        int value;
        Node* prev;
        Node* next;
        std::mutex nodeMutex;
        Node(int k, int v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };
    Node *head, *tail;
    Node *dummy_node_1, *dummy_node_2;
    // map key to node pointer
    unordered_map<int, Node*> cacheMap;
    // Helper function to remove a node from the doubly linked list.
    // It acquires locks on the previous, current, and next nodes to ensure thread safety during pointer updates.
    // The node is then pointed to dummy nodes to prevent dangling pointers.
    void RemoveNode(Node* node) {
        std::unique_lock<std::mutex> lockPrev(node->prev->nodeMutex, std::defer_lock);
        std::unique_lock<std::mutex> lockNode(node->nodeMutex, std::defer_lock);
        std::unique_lock<std::mutex> lockNext(node->next->nodeMutex, std::defer_lock);
        std::lock(lockPrev, lockNode, lockNext);
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->next = dummy_node_1;
        node->prev = dummy_node_2;
    }
    // Helper function to add a node to the front of the list (immediately after head).
    // This marks the node as the Most Recently Used (MRU).
    // It locks the head, the node after head, and the new node to safely insert it.
    void AddToFront(Node* node) {
        std::unique_lock<std::mutex> lockHead(head->nodeMutex, std::defer_lock);
        std::unique_lock<std::mutex> lockHeadNext(head->next->nodeMutex, std::defer_lock);
        std::unique_lock<std::mutex> lockNode(node->nodeMutex, std::defer_lock);
        std::lock(lockHead, lockHeadNext, lockNode);
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }
    // Helper function to evict the Least Recently Used (LRU) node.
    // The LRU node is located just before the tail sentinel.
    // It removes the node from the list, erases it from the map, and frees memory.
    void EvictLastNode() {
        Node* lru = tail->prev;
        RemoveNode(lru);
        cacheMap.erase(lru->key);
        delete lru;
        cur_capacity_--;
    }
public:
    // Constructor: Initializes the cache with a specific capacity.
    // Sets up the dummy head and tail nodes for the doubly linked list.
    LRUCache(size_t capacity) : capacity_(capacity), cur_capacity_(0) {
        head = new Node(-1, -1);
        tail = new Node(-1, -1);
        dummy_node_1 = new Node(-1, -1); 
        dummy_node_2 = new Node(-1, -1);
        head->next = tail;
        tail->prev = head;
    }
    // Destructor: Cleans up all nodes in the linked list and the dummy nodes to prevent memory leaks.
    ~LRUCache() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        delete dummy_node_1;
        delete dummy_node_2;
    }
    // Returns the maximum capacity of the cache.
    size_t capacity() const override {
        return capacity_;
    }
    // Returns the current number of elements in the cache.
    // Uses a lock to ensure a consistent read of the size.
    size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return cur_capacity_;
    }
    // Retrieves the value associated with the key.
    // If found, the node is moved to the front (MRU) and its value is returned.
    // If not found, returns -1.
    // Thread-safe via global mutex.
    int get(int key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cacheMap.find(key) == cacheMap.end()) {
            return -1;
        }
        Node* node = cacheMap[key];
        RemoveNode(node);
        AddToFront(node);
        return node->value;
    }
    // Inserts or updates a key-value pair.
    // If the key exists, updates the value and moves it to the front.
    // If the key is new, inserts it at the front. If capacity is exceeded, evicts the LRU node.
    // Thread-safe via global mutex.
    void put(int key, int value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cacheMap.find(key) != cacheMap.end()) {
            Node* node = cacheMap[key];
            node->value = value;
            RemoveNode(node);
            AddToFront(node);
        } else {
            if (cur_capacity_ == capacity_) {
                // evict the least recently used node
                EvictLastNode();
            }
            Node* newNode = new Node(key, value);
            cacheMap[key] = newNode;
            AddToFront(newNode);
            cur_capacity_++;
        }
    }
};

int main() {
    // Create a cache with capacity 2
    LRUCache cache(2);

    cout << "Putting (1, 10)" << endl;
    cache.put(1, 10);

    cout << "Putting (2, 20)" << endl;
    cache.put(2, 20);

    cout << "Getting key 1: " << cache.get(1) << endl; // Returns 10, moves 1 to front

    cout << "Putting (3, 30)" << endl;
    cache.put(3, 30); // Evicts key 2 (LRU) because 1 was recently used

    cout << "Getting key 2: " << cache.get(2) << endl; // Returns -1 (not found)

    cout << "Putting (4, 40)" << endl;
    cache.put(4, 40); // Evicts key 1 (LRU)

    cout << "Getting key 1: " << cache.get(1) << endl; // Returns -1 (not found)
    cout << "Getting key 3: " << cache.get(3) << endl; // Returns 30
    cout << "Getting key 4: " << cache.get(4) << endl; // Returns 40

    return 0;
}

// Time Complexity: O(1) expected
// Space Complexity: O(capacity)
// Pattern: Design - LRUCache
// explanation:
/*
cacheMap is an unordered map from key to node, so get and put find a key in expected constant time.
The nodes are a doubly linked list with sentinel head and tail, so RemoveNode and AddToFront relink a fixed number of pointers.
A new put at full capacity evicts the node just before tail, which is the least recently used key.
A mutex guards get, put, and size, so those calls run one at a time.
The map and the list store one node per key, up to capacity.
*/
