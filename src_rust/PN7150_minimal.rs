// i2c_ds3231.rs - Sets and retrieves the time on a Maxim Integrated DS3231
// RTC using I2C.

use std::error::Error;
use std::thread;
use std::time::Instant;
use std::time::Duration;

use rppal::i2c::I2c;
use rppal::gpio::Gpio;
use rppal::gpio::Level::Low;
use rppal::gpio::Level::High;

fn main() -> Result<(), Box<dyn Error>> {
    let mut i2c = I2c::new()?;
    let mut ven = Gpio::new()?.get(5)?.into_output();
    let irq = Gpio::new()?.get(4)?.into_input();

    i2c.set_slave_address(0x28)?;

    ven.write(Low);
    thread::sleep(Duration::from_millis(100));
    ven.write(High);
    thread::sleep(Duration::from_millis(100));

    println!("IRQ: {}", irq.read()); thread::sleep(Duration::from_millis(10));
    println!("");

    let blocks = vec![
        vec![0x20u8, 0x00u8, 0x01u8, 0x00u8],
        vec![0x20u8, 0x01u8, 0x00u8],
        vec![0x2Fu8, 0x02u8, 0x00u8],
        vec![0x21u8, 0x03u8, 0x09u8, 0x04u8, 0x00u8, 0x01u8, 0x01u8, 0x01u8, 0x02u8, 0x01u8, 0x06u8, 0x01u8]
    ];

    for block in blocks {
        println!("WR: {:02X?}", &block[..]);
        i2c.write(&block[..])?;
        loop {
            let status = irq.read();
            println!("IRQ: {}", status); thread::sleep(Duration::from_millis(1));
            if status != Low {
                break;
            }
        }
        println!("");

        let mut answ1 = [0xFFu8; 3];
        let mut answ2 = [0xFFu8; 64];
        i2c.read(&mut answ1)?;
        if answ1[2] > 0 {
            let a2 = &mut answ2[0..answ1[2] as usize];
            i2c.read(a2)?;
            println!("RD: {:02X?} - {:02X?}", answ1, a2);
        } else {
            println!("RD: {:02X?}", answ1);
        }

        println!("IRQ: {}", irq.read()); thread::sleep(Duration::from_millis(10));
        println!("");
    }


    loop {
        let status = irq.read();
        println!("IRQ: {}", status); thread::sleep(Duration::from_millis(100));
        if status != Low {
            break;
        }
    }
    println!("");

    let mut answ1 = [0xFFu8; 3];
    let mut answ2 = [0xFFu8; 64];
    i2c.read(&mut answ1)?;
    if answ1[2] > 0 {
        let a2 = &mut answ2[0..answ1[2] as usize];
        i2c.read(a2)?;
        println!("RD: {:02X?} - {:02X?}", answ1, a2);
    } else {
        println!("RD: {:02X?}", answ1);
    }


    loop {
        let status = irq.read();
        println!("IRQ: {}", status); thread::sleep(Duration::from_millis(100));
        if status != Low {
            break;
        }
    }
    println!("");

    let mut answ1 = [0xFFu8; 3];
    let mut answ2 = [0xFFu8; 64];
    i2c.read(&mut answ1)?;
    if answ1[2] > 0 {
        let a2 = &mut answ2[0..answ1[2] as usize];
        i2c.read(a2)?;
        println!("RD: {:02X?} - {:02X?}", answ1, a2);
    } else {
        println!("RD: {:02X?}", answ1);
    }
//    i2c.set_slave_address(ADDR_DS3231)?;
//    for i in 0..16 {
//        let mut regs = [0u8; 16];
//        i2c.block_read((i*16) as u8, &mut regs)?;
//        println!("{}\t{:02X?}", i, regs);
//    }
    return Ok(());
}