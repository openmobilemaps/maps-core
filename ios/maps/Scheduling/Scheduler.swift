import Foundation
import MapCoreSharedModule

extension OperationQueue {
  convenience init(concurrentOperations: Int, qos: QualityOfService, queue: DispatchQueue? = nil) {
    self.init()
    qualityOfService = qos
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

  private let ioHighQueue = OperationQueue(concurrentOperations: 10, qos: .userInteractive)
  private let ioNormalQueue = OperationQueue(concurrentOperations: 10, qos: .default)
  private let ioLowQueue = OperationQueue(concurrentOperations: 10, qos: .background)

  private let computationHighQueue = OperationQueue(concurrentOperations: 10, qos: .userInteractive)
  private let computationNormalQueue = OperationQueue(concurrentOperations: 10, qos: .default)
  private let computationLowQueue = OperationQueue(concurrentOperations: 10, qos: .background)

  private let graphicsQueue = OperationQueue(concurrentOperations: 1, qos: .userInteractive, queue: .main)

  private let internalSchedulerQueue = DispatchQueue(label: "internalSchedulerQueue")


  private var outstandingOperations: [String: TaskOperation] = [:]

  func addTask(_ task: MCTaskInterface?) {
    guard let task = task else { return }

    let config = task.getConfig()
    let delay = TimeInterval(config.delay / 1000)

    internalSchedulerQueue.asyncAfter(deadline: .now() + delay) {

      self.cleanUpFinishedOutstandingOperations()


      if self.outstandingOperations[config.id] != nil {
        self.removeTask(config.id)
      }

      let operation = TaskOperation(task: task)

      self.outstandingOperations[config.id] = operation

      switch config.executionEnvironment {
      case .IO:
        switch config.priority {
        case .HIGH:
          self.ioHighQueue.addOperation(operation)
        case .NORMAL:
          self.ioNormalQueue.addOperation(operation)
        case .LOW:
          self.ioLowQueue.addOperation(operation)
        @unknown default:
          fatalError("unknown priority")
        }
      case .COMPUTATION:
        switch config.priority {
        case .HIGH:
          self.computationHighQueue.addOperation(operation)
        case .NORMAL:
          self.computationNormalQueue.addOperation(operation)
        case .LOW:
          self.computationLowQueue.addOperation(operation)
        @unknown default:
          fatalError("unknown priority")
        }
      case .GRAPHICS:
        self.graphicsQueue.addOperation(operation)
      @unknown default:
        fatalError("unexpected executionEnvironment")
      }
    }
  }

  func removeTask(_ id: String) {
    cleanUpFinishedOutstandingOperations()
    outstandingOperations[id]?.cancel()
  }

  func clear() {
    outstandingOperations.forEach {
      $1.cancel()
    }
    cleanUpFinishedOutstandingOperations()
  }

  func pause() {
    cleanUpFinishedOutstandingOperations()

    ioHighQueue.isSuspended = true
    ioNormalQueue.isSuspended = true
    ioLowQueue.isSuspended = true

    computationHighQueue.isSuspended = true
    computationNormalQueue.isSuspended = true
    computationLowQueue.isSuspended = true

    graphicsQueue.isSuspended = true
  }

  func resume() {
    cleanUpFinishedOutstandingOperations()

    ioHighQueue.isSuspended = false
    ioNormalQueue.isSuspended = false
    ioLowQueue.isSuspended = false

    computationHighQueue.isSuspended = false
    computationNormalQueue.isSuspended = false
    computationLowQueue.isSuspended = false

    graphicsQueue.isSuspended = false
  }

  func cleanUpFinishedOutstandingOperations() {
    outstandingOperations = outstandingOperations.filter {
      !($1.isCancelled)
    }
  }
}
