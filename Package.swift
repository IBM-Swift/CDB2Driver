import PackageDescription

let package = Package(
    name: "CDB2Driver",
    dependencies: [
      .Package(url: "https://github.com/rfdickerson/CODBC", majorVersion: 0, minor: 0)
    ]

)
