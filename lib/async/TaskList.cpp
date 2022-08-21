template <typename TimeType>
struct Task {
    void (*execute)();
    TimeType time;
    TimeType interval;
    bool once;
    bool isBlocked = false;
    Task(void (*_function)(), TimeType _interval, bool _once):
        execute(_function), interval(_interval), once(_once) {}
};

struct ListNode {};

template <typename TimeType>
struct TaskNode : ListNode {
    Task<TimeType> value;
    TaskNode<TimeType>* next;
    TaskNode(Task<TimeType> task): value(task), next(nullptr) {}
};

template <typename TimeType>
struct TaskList {
    TaskNode<TimeType>* first;
    TaskNode<TimeType>* last;
    unsigned int length = 0;

    TaskList(): first(nullptr), last(nullptr) {}

    TaskNode<TimeType>* push(Task<TimeType> task) {
        TaskNode<TimeType>* pointer = new TaskNode<TimeType>(task);
        if (first != nullptr) {
            last -> next = pointer;
            last = pointer;
        } else {
            first = pointer;
            last = pointer;
        }
        length++;
        return pointer;
    }

    void remove(ListNode* pointer) {
        if (first == nullptr) return;
        if (pointer == first) {
            removeFirst();
            return;
        }
        TaskNode<TimeType>* before = first;
        while (before -> next != pointer) {
            before = before -> next;
            if (before == nullptr) return;
        }
        removeNext(before);
    }

    void removeFirst() {
        TaskNode<TimeType>* target = first;
        first = target -> next;
        delete target;
        length--;
    }

    void removeNext(TaskNode<TimeType>* before) {
        TaskNode<TimeType>* target = before -> next;
        before -> next = target -> next;
        delete target;
        length--;
    }
};

// void remove(int index) {
//     TaskNode<TimeType>* beforeTarget = first;
//     if (index == 0) {
//         first = beforeTarget -> next;
//         delete beforeTarget;
//         length--;
//         return;
//     }
//     for (int i = 0; i < index - 1; i++) {
//         beforeTarget = beforeTarget -> next;
//     }
//     TaskNode<TimeType>* target = beforeTarget -> next;
//     beforeTarget -> next = target -> next;
//     delete target;
//     length--;
// }

// TaskNode<TimeType>* operator[](const int index) {
//     if (first == nullptr) return nullptr;
//     TaskNode<TimeType>* res = first;
//     for (int i = 0; i < index; i++) {
//         res = res -> next;
//         if (res == nullptr) return nullptr;
//     }
//     return res;
// }
