import PackageDescription

let package = Package(
    name: "CDB2Driver",
    dependencies: [
      .Package(url: "https://github.com/rfdickerson/CODBC/blob/master/Package.swift", majorVersion: 0, minor: 0)
    ]

)
