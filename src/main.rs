use clap::{App, Arg};
use std::fmt;

fn main() {
    let matches = App::new("deltaup-prototype")
        .version("0.0.0")
        .author("hiromi-mi")
        .about("delta update")
        .arg(Arg::with_name("original").index(1).required(false))
        .arg(Arg::with_name("modified").index(2).required(false))
        .get_matches();

    match matches.value_of("original") {
        Some(file) => println!("Hello, world!: {}", file),
        None => println!("no"),
    }
    match matches.value_of("modified") {
        Some(file) => println!("Hello : {}", file),
        None => println!("no modified file given."),
    }
}

struct EncodedProgram {
    program : Vec<EncodedProgramInstruction>,
}

impl EncodedProgram {
fn new() -> EncodedProgram {
    EncodedProgram { program: vec![] }
}
}

impl ToString for EncodedProgram {
    fn to_string(&self) -> String {
        "".to_string()
    }
}

#[derive(Debug)]
enum EncodedProgramInstruction {
    Copy,
    Add(u32, String)
}

impl fmt::Display for EncodedProgramInstruction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    write!(f, "{:?}", self)
    }
}
