#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <limits.h>
#include <unordered_map>
#include <iomanip>
#include <atomic>

#define STORE_FILE "store/dumpFile"
std::mutex mtx;
char delimiter=':';

using TimePoint=std::chrono::steady_clock::time_point;

template<typename K, typename V>
class SkipList;  // SkipList的前向声明

template<typename K,typename V>
class Node
{
    friend class SkipList<K, V>; //友好类
public:
    Node(){}
    Node(const K& k,const V& v,int level,int TTL_seconds=INT_MAX)
    {
        this->key=k;
        this->value=v;
        this->node_level=level;
        this->forward=new Node<K,V> *[level+1];
        this->next=nullptr;
        this->next=nullptr;
        memset(this->forward,0,sizeof(Node<K,V> *) *(level+1));
        set_expiration_time(TTL_seconds);
    }
    ~Node();
    K get_key() const;
    V get_value() const;
    void set_value(V);
    void update_expiration_time();
    void set_expiration_time(int);

private:
    Node<K,V> ** forward;
    Node<K,V>* next;
    Node<K,V>* prev;
    K key;
    V value;
    int node_level;  //节点所在最高层数
    TimePoint expiration_time; //过期时间
    int TTL_seconds; //存活时间
};


template <typename K,typename V>
class SkipList
{
private:
    int _max_level;   //跳表最大层数
    int _skip_list_level; //跳表当前层数
    int _element_count; //跳表中的所有节点数量
    Node<K,V> *_header; //跳表头结点
    std::ofstream _file_writer; //文件写入器
    std::ifstream _file_reader; //文件读取器
public:
    SkipList(int,int); //跳表最大层级和回收站容量
    ~SkipList();
    int get_random_level();  //获取节点的随机层
    Node<K,V> *create_node(K,V,int); //创建节点
    Node<K,V> *create_node(K,V,int,int); //创建节点,并设置过期时间
    int insert_element(K,V,int); //插入节点
    int insert_element(Node<K,V>* node); //元素插入函数重载
    void display_list();  //展示节点
    V* search_element(K);  //搜索节点
    V* search_from_RecycleBin(K k); //从回收站查询
    void delete_element(K k,int flag);  //删除节点,flag为1表示加锁，0表示不加锁，独立线程定期删除过期节点时使用flag=0
    bool restore_from_RecycleBin(K k);
    bool restore_all_from_RecycleBin();
    void dump_file();  //持久化数据到文件
    void load_file();  //从文件加载数据
    void clear(Node<K,V> *); //递归删除节点
    int get_KV_size();  //跳表的节点个数
    void set_max_level(int x);
    void get_key_value_from_string(const std::string &str, std::string *key, std::string *value,int &ttl_seconds);
    bool is_valid_string(const std::string &str);

//独立线程执行定期删除相关
private:
    std::thread taskThread; //任务线程
    std::atomic<bool> stopFlag; //独立线程停止标志
    int intervalSeconds; //独立线程执行操作的时间间隔
    void run();
    void set_interval_second(int intervalSeconds);
    void deletionTask();

public:
    void start_scheduled_deletion(int intervalSeconds); //启动独立线程
    void stop_scheduled_deletion(); //停止线程

//回收站相关
private:
    std::unordered_map<K,Node<K,V>*> recycle_bin;
    Node<K,V>* head;
    Node<K,V>* tail;
    int size;
    int capacity;
public:
    void insert_to_RecycleBin(Node<K,V>* cur);
    void addToHead(Node<K,V>* node);
    void removeNode(Node<K,V>* node);
    void moveToHead(Node<K,V>* node);
    Node<K,V>* removeTail();
    void display_RecycleBin();
};



template<typename K,typename V>
Node<K,V>::~Node()
{
    delete[] forward;
}

template <typename K, typename V>
K Node<K, V>::get_key() const {
    return key;
};

template <typename K, typename V>
V Node<K, V>::get_value() const {
    return value;
};

template <typename K, typename V>
void Node<K, V>::set_value(V value) {
    this->value = value;
};

template <typename K, typename V>
inline void Node<K, V>::update_expiration_time()
{
    expiration_time=std::chrono::steady_clock::now() + std::chrono::seconds(TTL_seconds);
}

template <typename K, typename V>
inline void Node<K, V>::set_expiration_time(int ttl_seconds)
{
    TTL_seconds=ttl_seconds;
    expiration_time=std::chrono::steady_clock::now() + std::chrono::seconds(ttl_seconds);
}

template <typename K,typename V>
int SkipList<K,V>::get_random_level()
{
    int k=1; //初始化层级,每个节点至少出现在第一层
    while(rand()%2) ++k;
    k=(k<_max_level)?k:_max_level;
    return k;
}

template<typename K,typename V>
SkipList<K,V>::SkipList(int max_level,int capacity)
{
    this->_max_level=max_level; //最大层级
    this->_skip_list_level=0; //当前层级
    this->_element_count=0; //初始化节点计数
    this->capacity=capacity;
    K k;
    V v;
    //回收站虚拟头结点和尾节点
    head=new Node<K,V>(k,v,0);
    tail=new Node<K,V>(k,v,0);
    head->next=tail;
    tail->prev=head;
    // 创建头节点，并初始化键值为默认值
    this->_header=new Node<K,V>(k,v,_max_level);
}

template<typename K,typename V>
Node<K,V> *SkipList<K,V>::create_node(const K k,const V v,int level)
{
    Node<K,V> *n=new Node<K,V>(k,v,level);
    return n;
}

template<typename K,typename V>
Node<K,V> *SkipList<K,V>::create_node(const K k,const V v,int level,int ttl_seconds)
{
    Node<K,V> *n=new Node<K,V>(k,v,level,ttl_seconds);
    return n;
}

template <typename K,typename V>
V* SkipList<K,V>::search_element(K key)
{
    Node<K,V> *current=_header;
    for(int i=_skip_list_level;i>=0;--i)
    {
        while(current->forward[i] && current->forward[i]->get_key() < key)
        {
            current=current->forward[i];
        }
    }
    current=current->forward[0];
    if(current && current->get_key()==key) 
    {
        if(std::chrono::steady_clock::now()<current->expiration_time)
        {
            current->update_expiration_time();
            return &(current->value);
        }else
        {
            delete_element(key,1);
            return nullptr;
        }
    }
    return nullptr;
}

template <typename K,typename V>
int SkipList<K,V>::insert_element(const K key,const V value,int ttl_seconds)
{
    mtx.lock();
    bool flag=false;
    Node<K,V> *current=this->_header;
    Node<K,V> *update[_max_level+1];
    memset(update,0,sizeof(Node<K,V> *) * (_max_level+1));

    for(int i=_skip_list_level;i>=0;--i)
    {
        while(current->forward[i]!=NULL && current->forward[i]->get_key()<key)
        {
            current=current->forward[i];
        }
        update[i]=current;
    }

    current=current->forward[0];
    if(current!=NULL && current->get_key()==key) 
    {
        if(current->expiration_time>std::chrono::steady_clock::now())
        {
            // std::cout<<"key: "<<key<<", exists"<<std::endl;
            mtx.unlock();
            return 1;
        }else
        {
            flag=true; //表示存在过期键值对未删除，删除后添加一个新的键值对
            delete_element(current->key,0);
        }
    }
    if(current==NULL || current->get_key()!=key || flag)
    {
        int random_level=get_random_level();
        if(random_level > _skip_list_level)
        {
            for(int i=_skip_list_level+1;i<=random_level;++i)
            {
                update[i]=_header;
            }
            _skip_list_level=random_level;
        }
        Node<K,V> *inserted_node=create_node(key,value,random_level,ttl_seconds);
        for(int i=0;i<random_level;++i)
        {
            inserted_node->forward[i]=update[i]->forward[i];
            update[i]->forward[i]=inserted_node;
        }
        _element_count++;
    }
    mtx.unlock();
    return 0;
}

template<typename K,typename V>
void SkipList<K,V>::delete_element(K key,int flag)
{
    if(flag==1) mtx.lock();
    Node<K,V> *current=this->_header;
    Node<K,V> *update[_max_level+1];
    memset(update,0,sizeof(Node<K,V> *)*(_max_level+1));

    for(int i=_skip_list_level;i>=0;--i)
    {
        while(current->forward[i]!=NULL && current->forward[i]->get_key()<key)
        {
            current=current->forward[i];
        }
        update[i]=current;
    }

    current=current->forward[0];
    if(current!=NULL && current->get_key()==key)
    {
        for(int i=0;i<=_skip_list_level;++i)
        {
            if(update[i]->forward[i]!=current) break;
            update[i]->forward[i]=current->forward[i];
        }
        while(_skip_list_level>0 && _header->forward[_skip_list_level]==NULL)
        {
            --_skip_list_level;
        }
        insert_to_RecycleBin(current);
        // delete current;
        --_element_count;
    }
    if(flag==1) mtx.unlock();
}


template<typename K,typename V>
void SkipList<K,V>::display_list()
{
    mtx.lock();
    deletionTask();
    mtx.unlock();
    for(int i=_skip_list_level;i>=0;--i)
    {
        Node<K,V> *node=this->_header->forward[i];
        std::cout<<"Level "<<i<<": ";
        while(node!=nullptr)
        {
            std::cout<<node->get_key()<<"->"<<node->get_value()<<"; ";
            node=node->forward[i];
        }
        std::cout<<std::endl;
    }
}

template <typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
    return !str.empty() && str.find(delimiter) != std::string::npos;
}

template <typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value,int &ttl_seconds) {
    if (!is_valid_string(str)) {
        return;
    }
    int idx1=str.find(delimiter);
    int idx2=str.rfind(delimiter);
    *key = str.substr(0, idx1);
    *value = str.substr(idx1 + 1,idx2-idx1-1);
    ttl_seconds=stoi(str.substr(idx2+1));
}

template <typename K, typename V>
void SkipList<K, V>::load_file() {
    _file_reader.open(STORE_FILE);
    std::string line;
    std::string *key = new std::string();
    std::string *value = new std::string();
    int ttl_seconds;

    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value,ttl_seconds);
        if (key->empty() || value->empty()) {
            continue;
        }
        // Define key as int type
        insert_element(stoi(*key), *value,ttl_seconds);
        std::cout << "key:" << *key << "value:" << *value<<"TTL_seconds:"<< ttl_seconds<< std::endl;
    }

    delete key;
    delete value;
    _file_reader.close();
}

template <typename K, typename V>
void SkipList<K, V>::dump_file() {
    _file_writer.open(STORE_FILE); // 打开文件
    Node<K, V>* node = this->_header->forward[0]; // 从头节点开始遍历

    while (node != nullptr) {
        _file_writer << node->get_key() << ":" << node->get_value() <<":"<<node->TTL_seconds<< ";\n"; // 写入键值对
        // std::cout << node->get_key() << ":" << node->get_value() <<":"<<node->TTL_seconds<< ";\n"; // 写入键值对
        node = node->forward[0]; // 移动到下一个节点
    }

    _file_writer.flush(); // 刷新缓冲区，确保数据完全写入
    _file_writer.close(); // 关闭文件
}

template<typename K, typename V> 
SkipList<K, V>::~SkipList() {
    stop_scheduled_deletion();
    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }

    //递归删除跳表链条
    if(_header->forward[0]!=nullptr){
        clear(_header->forward[0]);
    }
    delete(_header);
}
template <typename K, typename V>
void SkipList<K, V>::clear(Node<K, V> * cur)
{
    if(cur->forward[0]!=nullptr){
        clear(cur->forward[0]);
    }
    delete(cur);
}

template<class K, class V>
void SkipList<K, V>::set_max_level(int x)
{
    this->_max_level=x;
}

template <typename K, typename V>
inline void SkipList<K, V>::insert_to_RecycleBin(Node<K, V> *cur)
{
    K key=cur->key;
    if(!recycle_bin.count(key))
    {
        recycle_bin[key]=cur;
        addToHead(cur);
        ++size;
        if(size>capacity)
        {
            Node<K,V>* removed=removeTail();
            recycle_bin.erase(removed->key);
            delete removed;
            --size;
        }
    }
    else
    {
        Node<K,V>* node=recycle_bin[key];
        node->value=cur->value;
        moveToHead(node);
    }
}

template <typename K, typename V>
inline void SkipList<K, V>::addToHead(Node<K, V> *node)
{
    node->prev=head;
    node->next=head->next;
    head->next->prev=node;
    head->next=node;
}

template <typename K, typename V>
inline void SkipList<K, V>::removeNode(Node<K, V> *node)
{
    node->prev->next=node->next;
    node->next->prev=node->prev;
}

template <typename K, typename V>
inline void SkipList<K, V>::moveToHead(Node<K, V> *node)
{
    removeNode(node);
    addToHead(node);
}

template <typename K, typename V>
inline Node<K, V> *SkipList<K, V>::removeTail()
{
    Node<K,V>* node=tail->prev;
    removeNode(node);
    return node;
}

template <typename K, typename V>
inline void SkipList<K, V>::display_RecycleBin()
{
    Node<K,V>* cur=head->next;
    if(cur==tail)
    {
        std::cout<<"Recycle Bin is Empty!"<<std::endl;
        return ;
    }
    std::cout<<"-----------Recycle Bin Display-----------"<<std::endl;
    std::cout<<"(Sort in descending order by recent visit time)"<<std::endl;
    std::cout<<std::left<<std::setw(10)<<"Key"<<std::setw(10)<<"Value"<<std::endl;
    while(cur!=tail)
    {
       std::cout<<std::left<<std::setw(10)<<cur->get_key()<<std::setw(10)<<cur->get_value()<<std::endl;
       cur=cur->next;
    }
}

template <typename K, typename V>
inline int SkipList<K, V>::get_KV_size()
{
    return _element_count;
}


template <typename K, typename V>
inline V* SkipList<K, V>::search_from_RecycleBin(K k)
{
    if(recycle_bin.count(k)){
        moveToHead(recycle_bin[k]);
        return &(recycle_bin[k]->value);
    }else
    {
        return nullptr;
    }
}

template <typename K, typename V>
inline bool SkipList<K, V>::restore_from_RecycleBin(K k)
{
    if(insert_element(recycle_bin[k])==0){
        removeNode(recycle_bin[k]);
        recycle_bin.erase(k);
        --size;
        return true;
    }
    return false;
}

template <typename K, typename V>
inline bool SkipList<K, V>::restore_all_from_RecycleBin()
{
    for(auto &t:recycle_bin)
    {
        if(search_element(t.first)!=nullptr) return false;
    }
    for(auto &t:recycle_bin)
    {
        insert_element(t.second);
    }
    head->next=tail;
    tail->prev=head;
    recycle_bin.clear();
    size=0;
    return true;
}

template <typename K, typename V>
inline int SkipList<K, V>::insert_element(Node<K, V> *inserted_node)
{
    K key=inserted_node->key;

    Node<K, V> *current = this->_header;
    Node<K, V> *update[_max_level + 1];  // 用于记录每层中待更新指针的节点
    memset(update, 0, sizeof(Node<K, V> *) * (_max_level + 1));

    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i]; // 移动到下一节点
        }
        update[i] = current;
    }
    current = current->forward[0];
    if (current != NULL && current->get_key() == key) {
        return 1;
    }
    if (current == NULL || current->get_key() != key) {
        inserted_node->update_expiration_time(); //更新到期时间
        int random_level = get_random_level();
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level + 1; i <= random_level; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        _element_count++;
    }
    return 0;
}

template <typename K, typename V>
inline void SkipList<K, V>::start_scheduled_deletion(int intervalSeconds)
{
    stopFlag.store(false);
    this->intervalSeconds=intervalSeconds;
    taskThread=std::thread(&SkipList<K,V>::run,this);
}

template <typename K, typename V>
inline void SkipList<K, V>::stop_scheduled_deletion()
{
    stopFlag.store(true);
    if(taskThread.joinable())
    {
        taskThread.join();
    }
}

template <typename K, typename V>
inline void SkipList<K, V>::run()
{
    while (!stopFlag.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));  // 等待间隔时间
        if (!stopFlag.load()) {
            mtx.lock();
            deletionTask();
            dump_file();
            mtx.unlock();
        }
    }
}

template <typename K, typename V>
inline void SkipList<K, V>::set_interval_second(int intervalSeconds)
{
    this->intervalSeconds=intervalSeconds;
}

template <typename K, typename V>
inline void SkipList<K, V>::deletionTask()
{
    Node<K,V>* cur=_header->forward[0];
    while(cur!=NULL)
    {
        Node<K,V>* tmp=cur;
        cur=cur->forward[0];
        if(tmp->expiration_time<=std::chrono::steady_clock::now())
        {
            delete_element(tmp->key,0);
        }
    }
}

#endif
