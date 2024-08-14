// �����Ҫ��ͷ�ļ�
#include <iostream> // �������������
#include <chrono> // ���ڸ߾���ʱ�����
#include <cstdlib> // ����һЩͨ�õĹ��ߺ����������������
#include <pthread.h> // ���ڶ��̱߳��
#include <time.h> // ����ʱ�䴦����
#include "./skiplist.h" // �����Զ��������ʵ��

// ����곣��
#define NUM_THREADS 5 // �߳�����
#define TEST_COUNT 100000 // �����õ���������С
SkipList<int, std::string> skipList(18,10000); // ����һ�����㼶Ϊ18������ʵ��

// ����Ԫ�ص��̺߳���
void *insertElement(void* threadid) {
    long tid; // �߳�ID
    tid = static_cast<long>(reinterpret_cast<uintptr_t>(threadid));// ��void*���͵��߳�IDת��Ϊlong��
    std::cout << tid << std::endl; // ����߳�ID
    int tmp = TEST_COUNT/NUM_THREADS; // ����ÿ���߳�Ӧ�ò����Ԫ������
    // ѭ������Ԫ��
    for (int i=tid*tmp, count=0; count<tmp; i++) {
        count++;
        skipList.insert_element(rand() % TEST_COUNT, "a",rand()%1000); // �������һ���������������"a"��Ԫ��
    }
    pthread_exit(NULL); // �˳��߳�
}

// ����Ԫ�ص��̺߳���
void *getElement(void* threadid) {
    long tid; // �߳�ID
    tid = static_cast<long>(reinterpret_cast<uintptr_t>(threadid)); // ��void*���͵��߳�IDת��Ϊlong��
    std::cout << tid << std::endl; // ����߳�ID
    int tmp = TEST_COUNT/NUM_THREADS; // ����ÿ���߳�Ӧ�ü�����Ԫ������
    // ѭ������Ԫ��
    for (int i=tid*tmp, count=0; count<tmp; i++) {
        count++;
        skipList.search_element(rand() % TEST_COUNT); // �������һ�����������Լ���
    }
    pthread_exit(NULL); // �˳��߳�
}

int main() {
    srand(time(NULL)); // ��ʼ�������������
    {
        pthread_t threads[NUM_THREADS]; // �����߳�����
        int rc; // ���ڽ���pthread_create�ķ���ֵ
        int i; // ѭ��������

        auto start = std::chrono::high_resolution_clock::now(); // ��ʼ��ʱ

        // ��������Ԫ�ص��߳�
        for( i = 0; i < NUM_THREADS; i++ ) {
            std::cout << "main() : creating thread, " << i << std::endl;
            rc = pthread_create(&threads[i], NULL, insertElement, (void *)i); // �����߳�

            if (rc) {
                std::cout << "Error:unable to create thread," << rc << std::endl;
                exit(-1); // ����̴߳���ʧ�ܣ��˳�����
            }
        }

        void *ret; // ���ڽ���pthread_join�ķ���ֵ
        // �ȴ����в����߳����
        for( i = 0; i < NUM_THREADS; i++ ) {
            if (pthread_join(threads[i], &ret) != 0 )  {
                perror("pthread_create() error");
                exit(3); // ����̵߳ȴ�ʧ�ܣ��˳�����
            }
        }
        auto finish = std::chrono::high_resolution_clock::now(); // ������ʱ
        std::chrono::duration<double> elapsed = finish - start; // �����ʱ
        std::cout << "insert elapsed:" << elapsed.count() << std::endl; // ������������ʱ
    }

    // ����Ĵ�������������ƣ����ڴ�������������������߳�
    {
        pthread_t threads[NUM_THREADS];
        int rc;
        int i;
        auto start = std::chrono::high_resolution_clock::now();

        for( i = 0; i < NUM_THREADS; i++ ) {
            std::cout << "main() : creating thread, " << i << std::endl;
            rc = pthread_create(&threads[i], NULL, getElement, (void *)i);

            if (rc) {
                std::cout << "Error:unable to create thread," << rc << std::endl;
                exit(-1);
            }
        }

        void *ret;
        for( i = 0; i < NUM_THREADS; i++ ) {
            if (pthread_join(threads[i], &ret) != 0 )  {
                perror("pthread_create() error");
                exit(3);
            }
        }

        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        std::cout << "get elapsed:" << elapsed.count() << std::endl;
    }

    pthread_exit(NULL); // ���߳��˳�

    return 0;
}