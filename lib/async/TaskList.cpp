template <typename TimeType>
struct Task {
  bool (*execute)();
  TimeType time;
  TimeType interval;
  bool once;
  bool isBlocked = false;
  Task(bool (*_function)(), TimeType _interval, bool _once):
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
  // unsigned int length = 0;

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
    // length++;
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
    // length--;
  }

  void removeNext(TaskNode<TimeType>* before) {
    TaskNode<TimeType>* target = before -> next;
    before -> next = target -> next;
    if (last == target) last = before;
    delete target;
    // length--;
  }
};
