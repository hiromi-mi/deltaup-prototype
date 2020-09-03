use clap::{App, Arg};

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

struct Architecture {

}
