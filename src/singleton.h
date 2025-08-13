#ifndef SINGLETON_H
#define SINGLETON_H

template <class T> class CSingleton {
  public:
    static T& GetInstance() {
        static T instance;
        return instance;
    }

    CSingleton(const CSingleton&) = delete;
    CSingleton& operator=(const CSingleton&) = delete;
    CSingleton(CSingleton&&) = delete;
    CSingleton& operator=(CSingleton&&) = delete;

  private:
    CSingleton() = default;
    ~CSingleton() = default;
};
#endif
