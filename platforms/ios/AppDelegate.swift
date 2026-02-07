//
//  AppDelegate.swift
//  OnDeviceAI Example App
//
//  App delegate for example application
//

import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
    
    var window: UIWindow?
    
    func application(
        _ application: UIApplication,
        didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
    ) -> Bool {
        
        // Create window
        let window = UIWindow(frame: UIScreen.main.bounds)
        
        // Create root navigation controller with example VC
        let exampleVC = ExampleViewController()
        let navController = UINavigationController(rootViewController: exampleVC)
        
        window.rootViewController = navController
        window.makeKeyAndVisible()
        
        self.window = window
        
        return true
    }
    
    func applicationDidEnterBackground(_ application: UIApplication) {
        // Handle background transition
        // SDK will automatically handle lifecycle events if configured
    }
    
    func applicationWillEnterForeground(_ application: UIApplication) {
        // Handle foreground transition
        // SDK will automatically handle lifecycle events if configured
    }
    
    func applicationDidReceiveMemoryWarning(_ application: UIApplication) {
        // SDK will automatically handle memory warnings if configured
    }
}
