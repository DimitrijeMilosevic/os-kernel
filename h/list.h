#ifndef _list_h_
#define _list_h_

class PCB;

template <class T>
class List {
public:
	struct Node {
		T data;
		Node* next;
		Node(T _data) : data(_data), next(0) {}
	};

	List();
	void add(T newElement);
	T removeFirst();
	void remove(T element);
	int numberOfElements() const;
	~List();

	friend void interrupt timer(...);

	friend class KernelSem;
	friend class PCB;
private:

	Node *first, *last;

};

template <class T>
List<T>::List() : first(0), last(0) {}

template <class T>
void List<T>::add(T newElement) {

	Node* newElem = new Node(newElement);
	if (first == 0) first = newElem;
	else last->next = newElem;
	last = newElem;

}

template <class T>
T List<T>::removeFirst() {

	if (first == 0) return 0;
	else {

		T data = first->data;
		Node* old = first;
		first = first->next;
		if (first == 0) last = 0;
		delete old;
		return data;

	}

}

template <class T>
void List<T>::remove(T element) {

	Node *current = first, *previous = 0;
	while ((current != 0) && (current->data != element)) {
		previous = current;
		current = current->next;
	}
	if (current == 0) return;
	Node* old = current;
	if (previous == 0) {
		first = first->next;
		if (first == 0) last = 0;
	}
	else {
		previous->next = current->next;
		if (current->next == 0) last = previous;
	}
	delete old;

}

template <class T>
int List<T>::numberOfElements() const {

	int numberOfElements = 0;
	Node* current = first;
	while (current != 0) {
		numberOfElements++;
		current = current->next;
	}
	return numberOfElements;

}

template <class T>
List<T>::~List() {

	while (first != 0) {
		Node* old = first;
		first = first->next;
		delete old;
	}

}

#endif
