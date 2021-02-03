import Foundation
import MapCoreSharedModule

class Loader: MCLoaderInterface {

    func loadDate(_ url: String) -> Data? {
        let urlString = url
        guard let url = URL(string: urlString) else {
            preconditionFailure("invalid url: \(urlString)")
        }
        let semaphore = DispatchSemaphore(value: 0)

        var result: Data?

        let task = URLSession.shared.dataTask(with: url) {(data, response, error) in
            result = data
            semaphore.signal()
        }

        task.resume()
        semaphore.wait()

        return result
    }
}
