// SPDX-License-Identifier: Apache-2.0

use clap::{App, Arg};
use std::fmt;

use std::io::prelude::*;
use std::fs::File;
use std::io::{BufRead, BufReader};

fn main() {
    let matches = App::new("deltaup-prototype")
        .version("0.0.0")
        .author("hiromi-mi")
        .about("delta update")
        .arg(Arg::with_name("original").index(1).required(false))
        .arg(Arg::with_name("modified").index(2).required(false))
        .get_matches();

    let original_fname = matches.value_of("original").expect("Original File Not Found");
    let modified_fname = matches.value_of("modified").expect("Modified File Not Found");

    let mut original_f = File::open(original_fname).expect("Original File Open Failed");
    //let original_reader = BufReader::new(original_f);
    let mut modified_f = File::open(modified_fname).expect("Modified File Open Failed");
    //let modified_reader = BufReader::new(modified_f);

    let mut orig = Vec::new();
    original_f.read_to_end(&mut orig).unwrap();
    let mut modi = Vec::new();
    modified_f.read_to_end(&mut modi).unwrap();
}

fn split(ic: &mut Vec<isize>, v: &mut Vec<u32>, start: usize, len: isize, h: u32) {
}

fn qsufsort(ic: &mut Vec<isize>, v: &mut Vec<u32>, old: &mut Vec<u32>, oldsize: usize) {
    let mut buckets = [0u32;256];
    // let mut i = 0usize;
    let mut h = 0u32;
    let mut len = 0;

    for j in old.iter() {
        buckets[*j as usize] += 1;
    }

    for j in 1..256 {
        buckets[j as usize] += buckets[j as usize - 1];
    }

    for j in (1..256).rev() {
        buckets[j as usize] = buckets[j as usize - 1];
    }
    buckets[0] = 0;
    // きれいに書きたい
    /*
    for bucket_i in &mut buckets.iter_mut() {
        *bucket_i = buckets_i_1
    }
    */

    // old file のバイト数のうち, buckets[c] は c より小さいもののリストになる

    for (index, j) in old.iter_mut().enumerate() {
        *j += 1;
        ic[*j as usize] = index as isize;
    }
    ic[0] = oldsize as isize;

    /* ? */
    for (index, vi) in v.iter_mut().enumerate() {
        *vi = buckets[old[index] as usize];
    }
    v[oldsize] = 0;

    for index in 1..256 {
        if buckets[index] == buckets[index-1] + 1 {
            ic[buckets[index] as usize] = -1;
        }
    }
    ic[0] = -1;

    while -ic[0] != oldsize as isize + 1 {
        len = 0;
        let mut i = 0;
        while i < oldsize + 1 {
            if ic[i] < 0 {
                len -= ic[i];
                i -= ic[i] as usize;
            } else {
                if len != 0 {
                    ic[i-len as usize] = (-len) as isize;
                }
                len = v[ic[i as usize] as usize] as isize + 1 - i as isize;
                split(ic, v, i, len, h);
                i += len as usize;
                len = 0
            }
        }
        if len != 0 {
            ic[i-len as usize] = -len;
        }
    }
}

struct EncodedProgram {
    program: Vec<EncodedProgramInstruction>,
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
    Insert(Vec<u32>),
    Add(u32, u32),
}

impl fmt::Display for EncodedProgramInstruction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}
