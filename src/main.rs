// Copyright 2018 the Deno authors. All rights reserved. MIT license.
extern crate flatbuffers;
#[macro_use]
extern crate futures;
extern crate hyper;
extern crate libc;
extern crate msg_rs as msg;
extern crate rand;
extern crate tempfile;
extern crate tokio;
extern crate tokio_executor;
extern crate tokio_fs;
extern crate tokio_io;
extern crate tokio_threadpool;
extern crate url;
#[macro_use]
extern crate lazy_static;
#[macro_use]
extern crate log;
extern crate dirs;
extern crate getopts;
extern crate hyper_rustls;
extern crate remove_dir_all;
extern crate ring;

mod deno_dir;
mod errors;
mod flags;
mod fs;
mod http_util;
mod isolate;
mod libdeno;
pub mod ops;
mod resources;
mod snapshot;
mod tokio_util;
mod tokio_write;
mod version;

#[cfg(unix)]
mod eager_unix;

use std::env;

static LOGGER: Logger = Logger;

struct Logger;

impl log::Log for Logger {
  fn enabled(&self, metadata: &log::Metadata) -> bool {
    metadata.level() <= log::max_level()
  }

  fn log(&self, record: &log::Record) {
    if self.enabled(record.metadata()) {
      println!("{} RS - {}", record.level(), record.args());
    }
  }
  fn flush(&self) {}
}

fn main() {
  // Rust does not die on panic by default. And -Cpanic=abort is broken.
  // https://github.com/rust-lang/cargo/issues/2738
  // Therefore this hack.
  std::panic::set_hook(Box::new(|panic_info| {
    if let Some(location) = panic_info.location() {
      eprintln!("PANIC file '{}' line {}", location.file(), location.line());
    } else {
      eprintln!("PANIC occurred but can't get location information...");
    }
    std::process::abort();
  }));

  log::set_logger(&LOGGER).unwrap();
  let args = env::args().collect();
  let (flags, rest_argv, usage_string) =
    flags::set_flags(args).unwrap_or_else(|err| {
      eprintln!("{}", err);
      std::process::exit(1)
    });
  let mut isolate = isolate::Isolate::new(flags, rest_argv, ops::dispatch);
  flags::process(&isolate.state.flags, usage_string);
  tokio_util::init(|| {
    isolate
      .execute("deno_main.js", "denoMain();")
      .unwrap_or_else(|err| {
        error!("{}", err);
        std::process::exit(1);
      });
    isolate.event_loop();
  });
}
