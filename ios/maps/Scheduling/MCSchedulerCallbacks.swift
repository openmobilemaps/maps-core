//
//  MCSchedulerCallbacks.swift
//
//
//  Created by Stefan Mitterrutzner on 07.02.23.
//

import Foundation
import MapCoreSharedModule

public class MCSchedulerCallbacks: MCThreadPoolCallbacks {
    public func getCurrentThreadName() -> String {
        Thread.current.name ?? ""
    }

    public func setCurrentThreadName(_ name: String) {
        Thread.current.name = name
    }

    public func setThreadPriority(_ priority: MCTaskPriority) {
        switch priority {
            case .LOW:
                Thread.current.threadPriority = 0
            case .NORMAL:
                Thread.current.threadPriority = 0.5
            case .HIGH:
                Thread.current.threadPriority = 1
            @unknown default:
                fatalError()
        }
    }

    public func attachThread() {
    }

    public func detachThread() {
    }
}

public extension MCThreadPoolScheduler {
    static func create() -> MCSchedulerInterface {
        MCThreadPoolScheduler.create(MCSchedulerCallbacks())!
    }
}
