use winit::event_loop::EventLoop;
use winit::application::ApplicationHandler;
use winit::window::{Window, WindowAttributes};


struct App {

    window: Option<Window>

}

// Implementing the "Default" trait

impl Default for App {
    fn default() -> Self {
        Self {
            window: None
        }
    }
}

// Implementing some core app related functions

impl App {

    fn initialize_window(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        self.window = match event_loop.create_window(WindowAttributes::default().with_title("Untitled Video Game")) {
            Ok(window) => Some(window),
            Err(msg) => {panic!("Failed to create window! Error: {msg}");}
        };
    }

}

// Implementing the application handler for winit
impl ApplicationHandler for App {

    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        // Checking if the window is created or not, if it isn't then we go ahead
        // and make one.
        if self.window.is_none() {
            // Creating a new window here.
            self.initialize_window(event_loop);
        }

    }

    fn window_event(
            &mut self,
            event_loop: &winit::event_loop::ActiveEventLoop,
            window_id: winit::window::WindowId,
            event: winit::event::WindowEvent,
        ) {

            match event {
                winit::event::WindowEvent::CloseRequested => event_loop.exit(),
                _ => {}
            }

    }

}

fn main() {
    let event_loop = match EventLoop::new() {
        Ok(event_loop) => event_loop,
        Err(msg) => {panic!("Can't initialize event loop! Error: {msg}");}
    };

    let mut app: App = App::default();

    match event_loop.run_app(&mut app) {
        Ok(_) => println!("App ran successfully!"),
        Err(msg) => panic!("App didn't run successfully! Error: {msg}")
    }

}
