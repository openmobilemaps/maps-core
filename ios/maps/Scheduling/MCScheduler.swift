/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation
import MapCoreSharedModule

private class WeakOperation {
    weak var operation: TaskOperation?

    init(_ operation: TaskOperation) {
        self.operation = operation
    }
}

open class MCScheduler: MCSchedulerInterface {
    private let ioQueue = OperationQueue(concurrentOperations: 64, qos: .userInitiated)

    private let computationQueue = OperationQueue(concurrentOperations: 20, qos: .userInitiated)

    private let graphicsQueue = OperationQueue.main

    private let internalSchedulerQueue = DispatchQueue(label: "internalSchedulerQueue")

    private var pendingOperations: [String: WeakOperation] = [:]

     public init() {
    }

    public func addTasks(_ tasks: [MCTaskInterface]) {
        tasks.forEach(addTask(_:))
    }

    public func addTask(_ task: MCTaskInterface?) {
        guard let task = task else { return }

        let config = task.getConfig()
        let delay = TimeInterval(Double(config.delay) / 1000.0)

        if config.executionEnvironment == .GRAPHICS && delay == 0.0 {
            task.run()
            return
        }

        internalSchedulerQueue.asyncAfter(deadline: .now() + delay) { [weak self] in
            guard let self = self else { return }

            let operation = TaskOperation(task: task, scheduler: self)

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

            self.pendingOperations[config.id] = .init(operation)

            operation.completionBlock = { [weak self] in
                guard let self = self else { return }
                self.internalSchedulerQueue.async { [weak self] in
                    guard let self = self else { return }
                    self.pendingOperations.removeValue(forKey: config.id)
                }
            }

            #if DEBUG
            operation.completionBlock = {
                print("MCScheduler: Queue Count After", self.ioQueue.operations.count, self.computationQueue.operations.count, self.graphicsQueue.operations.count)
            }
            #endif

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
            
            #if DEBUG
            print("queue count before", self.ioQueue.operations.count, self.computationQueue.operations.count, self.graphicsQueue.operations.count)
            #endif
        }
    }

    public func removeTask(_ id: String) {
        internalSchedulerQueue.async {
            self.pendingOperations[id]?.operation?.cancel()
            self.pendingOperations.removeValue(forKey: id)
        }
    }

    public func clear() {
        internalSchedulerQueue.async {
            self.pendingOperations.forEach {
                $1.operation?.cancel()
            }
            self.pendingOperations.removeAll()
        }
    }

    public func pause() {
        internalSchedulerQueue.async {
            self.ioQueue.isSuspended = true

            self.computationQueue.isSuspended = true

            self.graphicsQueue.isSuspended = true
        }
    }

    public func resume() {
        internalSchedulerQueue.async {
            self.ioQueue.isSuspended = false

            self.computationQueue.isSuspended = false

            self.graphicsQueue.isSuspended = false
        }
    }
}

private extension OperationQueue {
    convenience init(concurrentOperations: Int, queue: DispatchQueue? = nil, qos: QualityOfService = .default) {
        self.init()
        maxConcurrentOperationCount = concurrentOperations
        underlyingQueue = queue
        qualityOfService = qos
    }
}

private class TaskOperation: Operation {
    var task: MCTaskInterface
    weak var scheduler: MCScheduler?

    init(task: MCTaskInterface, scheduler: MCScheduler) {
        self.task = task
        self.scheduler = scheduler
    }

    override func main() {
        guard scheduler != nil else {
            return
        }
        task.run()
    }
}
