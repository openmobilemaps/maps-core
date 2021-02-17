import Foundation
import MapCoreSharedModule
import os

extension OperationQueue {
    convenience init(concurrentOperations: Int, queue: DispatchQueue? = nil) {
        self.init()
        maxConcurrentOperationCount = concurrentOperations
        underlyingQueue = queue
    }
}

class TaskOperation: Operation {
    var task: MCTaskInterface

    init(task: MCTaskInterface) {
        self.task = task
    }

    override func main() {
        task.run()
    }
}

class Scheduler: MCSchedulerInterface {
    private let ioQueue = OperationQueue(concurrentOperations: 10)

    private let computationQueue = OperationQueue(concurrentOperations: 4)

    private let graphicsQueue = OperationQueue(concurrentOperations: 1)

    private let internalSchedulerQueue = DispatchQueue(label: "internalSchedulerQueue")

    private var outstandingOperations: [String: TaskOperation] = [:]

    func addTask(_ task: MCTaskInterface?) {
        guard let task = task else { return }

        let config = task.getConfig()
        let delay = TimeInterval(config.delay / 1000)

        /*if #available(iOS 14.0, *) {
            os_log("dispatching Task \(config.id)")
        }*/
        internalSchedulerQueue.asyncAfter(deadline: .now() + delay) {
            self.cleanUpFinishedOutstandingOperations()

            let operation = TaskOperation(task: task)

            switch config.priority {
            case .HIGH:
                operation.queuePriority = .high
            case .NORMAL:
                operation.queuePriority = .normal
            case .LOW:
                operation.queuePriority = .low
            @unknown default:
                fatalError("unknown priority")
            }

            self.outstandingOperations[config.id] = operation

            switch config.executionEnvironment {
            case .IO:
                self.ioQueue.addOperation(operation)
            case .COMPUTATION:
                self.computationQueue.addOperation(operation)
            case .GRAPHICS:
                self.graphicsQueue.addOperation(operation)
            @unknown default:
                fatalError("unexpected executionEnvironment")
            }
        }
    }

    func removeTask(_ id: String) {
        internalSchedulerQueue.async {
            self.cleanUpFinishedOutstandingOperations()
            self.outstandingOperations[id]?.cancel()
        }
    }

    func clear() {
        internalSchedulerQueue.async {
            self.outstandingOperations.forEach {
                $1.cancel()
            }
            self.cleanUpFinishedOutstandingOperations()
        }
    }

    func pause() {
        internalSchedulerQueue.async {
            self.cleanUpFinishedOutstandingOperations()

            self.ioQueue.isSuspended = true

            self.computationQueue.isSuspended = true

            self.graphicsQueue.isSuspended = true
        }
    }

    func resume() {
        internalSchedulerQueue.async {
            self.cleanUpFinishedOutstandingOperations()

            self.ioQueue.isSuspended = false

            self.computationQueue.isSuspended = false

            self.graphicsQueue.isSuspended = false
        }
    }

    func cleanUpFinishedOutstandingOperations() {
        outstandingOperations = outstandingOperations.filter {
            !$1.isCancelled
        }
    }
}
