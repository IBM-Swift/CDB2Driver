import PackageDescription

let package = Package(
    name: "CDB2Driver",
    dependencies: [
      .Package(url: "git@github.ibm.com:IBM-Swift/CODBC.git", majorVersion: 0, minor: 0)
    ]

)
