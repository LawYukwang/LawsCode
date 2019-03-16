template <typename T>
class MemoryPool
{
public:
	MemoryPool(size_t size=EXPANSION_SIZE);
	~MemoryPool();

	inline void* alloc(size_t size);
	inline void free(void *someElement);
private:
	enum {EXPANSION_SIZE=32};

	MemoryPool<T> *next;
	void expandTheFreeList(int howMany=EXPANSION_SIZE);
};

template <typename T>
MemoryPool<T>::MemoryPool(size_t size)
{
	expandTheFreeList(size);
}

template <typename T>
MemoryPool<T>::~MemoryPool()
{
	delete next;
}

template <typename T>
void* MemoryPool<T>::alloc(size_t)
{
	if(!next)
		expandTheFreeList();
	MemoryPool<T> *head=next;
	next=head->next;
	return head;
}

template <typename T>
void MemoryPool<T>::free(void *doomed)
{
	MemoryPool<T>* head=static_cast<MemoryPool<T>*>(doomed);
	head->next=next;
	next=head;
}

template <typename T>
void MemoryPool<T>::expandTheFreeList(int howMany)
{
	size_t size=(sizeof(T)>sizeof(MemoryPool<T>*))?sizeof(T):sizeof(MemoryPool<T>*);
	MemoryPool<T> *runner=reinterpret_cast<MemoryPool<T>*>(new char[size]);
	next=runner;
	for(int i=0;i<howMany;i++)
	{
		runner->next=reinterpret_cast<MemoryPool<T>*>(new char[size]);
		runner=runner->next;
	}

	runner->next=nullptr;
}