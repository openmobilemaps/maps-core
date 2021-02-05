import Foundation
import MapCoreSharedModule

class Scheduler: MCSchedulerInterface {
    private let ioHighDispatchQueue = DispatchQueue(label: "ioHighQueue", qos: .userInitiated, attributes: .concurrent)
    private let ioNormalDispatchQueue = DispatchQueue(label: "ioNormalQueue", qos: .default, attributes: .concurrent)
    private let ioLowDispatchQueue = DispatchQueue(label: "ioLowQueue", qos: .background, attributes: .concurrent)

    private let computationHighDispatchQueue = DispatchQueue(label: "computationHighQueue", qos: .userInitiated, attributes: .concurrent)
    private let computationNormalDispatchQueue = DispatchQueue(label: "computationNormalQueue", qos: .default, attributes: .concurrent)
    private let computationLowDispatchQueue = DispatchQueue(label: "computationLowQueue", qos: .background, attributes: .concurrent)

    private let graphicsDispatchQueue = DispatchQueue.main

    struct WeakWorkItem {
        weak var workItem: DispatchWorkItem?
    }

    private var outstandingWorkItems: [String: WeakWorkItem] = [:]

    func addTask(_ task: MCTaskInterface?) {
        cleanUpFinishedOutstandingWorkItems()
        guard let task = task else { return }
        let config = task.getConfig()

        if outstandingWorkItems[config.id] != nil {
            self.removeTask(config.id)
        }

        let delay = TimeInterval( config.delay / 1000 )
        let workItem = DispatchWorkItem {
            task.run()
        }
        outstandingWorkItems[config.id] = .init(workItem: workItem)

        switch config.executionEnvironment {
        case .IO:
            switch config.priority {
            case .HIGH:
                ioHighDispatchQueue.asyncAfter(deadline: .now() + delay, execute: workItem)
            case .NORMAL:
                ioNormalDispatchQueue.asyncAfter(deadline: .now() + delay, execute: workItem)
            case .LOW:
                ioLowDispatchQueue.asyncAfter(deadline: .now() + delay, execute: workItem)
            @unknown default:
                fatalError("unknown priority")
            }
        case .COMPUTATION:
            switch config.priority {
            case .HIGH:
                computationHighDispatchQueue.asyncAfter(deadline: .now() + delay, execute: workItem)
            case .NORMAL:
                computationNormalDispatchQueue.asyncAfter(deadline: .now() + delay, execute: workItem)
            case .LOW:
                computationLowDispatchQueue.asyncAfter(deadline: .now() + delay, execute: workItem)
            @unknown default:
                fatalError("unknown priority")
            }
        case .GRAPHICS:
            graphicsDispatchQueue.asyncAfter(deadline: .now() + delay, execute: workItem)
        @unknown default:
            fatalError("unexpected executionEnvironment")
        }
    }

    func removeTask(_ id: String) {
        cleanUpFinishedOutstandingWorkItems()
        outstandingWorkItems[id]?.workItem?.cancel()
    }

    func clear() {
        outstandingWorkItems.forEach {
            $1.workItem?.cancel()
        }
        cleanUpFinishedOutstandingWorkItems()
    }

    func pause() {
        cleanUpFinishedOutstandingWorkItems()

        ioHighDispatchQueue.suspend()
        ioNormalDispatchQueue.suspend()
        ioLowDispatchQueue.suspend()

        graphicsDispatchQueue.suspend()

        computationLowDispatchQueue.suspend()
        computationNormalDispatchQueue.suspend()
        computationHighDispatchQueue.suspend()
    }

    func resume() {
        cleanUpFinishedOutstandingWorkItems()

        ioHighDispatchQueue.resume()
        ioNormalDispatchQueue.resume()
        ioLowDispatchQueue.resume()

        graphicsDispatchQueue.resume()

        computationLowDispatchQueue.resume()
        computationNormalDispatchQueue.resume()
        computationHighDispatchQueue.resume()
    }

    func cleanUpFinishedOutstandingWorkItems() {
        outstandingWorkItems = outstandingWorkItems.filter {
            !($1.workItem?.isCancelled ?? false) || $1.workItem != nil
        }
    }
}
