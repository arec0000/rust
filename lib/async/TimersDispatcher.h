#pragma once
#include "taskList.cpp"

typedef ListNode TaskId;

template <typename TimeType>
class TimersDispatcher {
  public:
    TimersDispatcher(TimeType (*timingFunction)(), TimeType maxTime):
      _timingFunction(timingFunction), _maxTime(maxTime) {}

    TaskId* setTimeout(bool (*callback)(), TimeType timeout) {
      return _taskList.push(_makeTask(callback, timeout, true));
    }

    TaskId* setInterval(bool (*callback)(), TimeType interval) {
      return _taskList.push(_makeTask(callback, interval, false));
    }

    void clearInterval(TaskId* pointer) {
      _taskList.remove(pointer);
    }

    void loop() {
      if (_taskList.first == nullptr) return;
      TaskNode<TimeType>* before = _taskList.first;
      TaskNode<TimeType>* node = _taskList.first;
      bool timeOverflowStage = false;

      while (node != nullptr) {

        TimeType time = _timingFunction();
        if (_lastTime > time) {
          before = _taskList.first;
          node = _taskList.first;
          timeOverflowStage = true;
        }
        _lastTime = time;

        if (node == _taskList.first) {
          bool isFinished = _checkTask(node -> value, timeOverflowStage);
          if (isFinished) {
            _taskList.removeFirst();
            node = _taskList.first;
            before = _taskList.first;
          } else {
            node = node -> next;
          }
        } else {
          bool isFinished = _checkTask(node -> value, timeOverflowStage);
          if (isFinished) {
            _taskList.removeNext(before);
            node = before -> next;
          } else {
            before = node;
            node = node -> next;
          }
        }

      }
    }

  private:
    TaskList<TimeType> _taskList;
    TimeType (*_timingFunction)();
    TimeType _maxTime;
    TimeType _lastTime;

      Task<TimeType> _makeTask(bool (*callback)(), TimeType interval, bool once) {
        Task<TimeType> task(callback, interval, once);
        if (_timingFunction() > _maxTime - interval) {
          task.time = _timingFunction() - (_maxTime - interval);
          task.isBlocked = true;
        } else {
          task.time = _timingFunction() + interval;
        }
        return task;
      }

      bool _checkTask(Task<TimeType>& task, bool timeOverflowStage) {
        if (!task.isBlocked && (_timingFunction() > task.time || timeOverflowStage)) {
          bool isFinished = task.execute();
          if (task.once || isFinished) {
            return true;
          }
          TimeType time = _timingFunction();
          if (time > _maxTime - task.interval) {
            task.time = time - (_maxTime - task.interval);
            task.isBlocked = true;
          } else {
            task.time = time + task.interval;
          }
        } else if (timeOverflowStage) {
          task.isBlocked = false;
        }
        return false;
      }

};
