use std::error::Error;
use std::thread;
use std::time::Instant;
use std::time::Duration;

use rppal::i2c::I2c;
use rppal::gpio::Gpio;
use rppal::gpio::Level::Low;
use rppal::gpio::Level::High;

#[derive(Debug)]
pub enum TmlError {
    Init,
    Read,
    Write,
    Timeout
}

impl std::error::Error for TmlError {}
impl std::fmt::Display for TmlError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("{:?}", self))
    }
}

pub struct Tml {
    i2c: rppal::i2c::I2c,
    irq: rppal::gpio::InputPin,
    ven: rppal::gpio::OutputPin
}

trait TmlTrait {
    fn connect() -> Result<Self, TmlError> where Self: Sized;
    fn disconnect(&mut self);
    fn reset(&mut self);
    fn send(&mut self, buffer: &[u8]) -> Result<usize, TmlError>;
    fn receive(&mut self, buffer: &mut [u8], timeout:std::time::Duration) -> Result<usize, TmlError>;
}

impl TmlTrait for Tml {
    fn connect() -> Result<Self, TmlError> where Self: Sized {
        let dev_i2c = I2c::with_bus(1);
        let io = Gpio::new();

        if dev_i2c.is_ok() && io.is_ok()  {
            let mut dev_i2c = dev_i2c.unwrap();
            if dev_i2c.set_slave_address(0x28).is_ok() {
                let port_irq = io.as_ref().unwrap().get(4);
                let port_ven = io.unwrap().get(5);
                if port_ven.is_ok() && port_irq.is_ok() {
                    return Ok(Tml {
                            i2c:dev_i2c,
                            irq:port_irq.unwrap().into_input(),
                            ven:port_ven.unwrap().into_output()
                        });
                } else {
                    return Err(TmlError::Init);
                }
            } else {
                return Err(TmlError::Init);
            }
        } else {
            return Err(TmlError::Init);
        }
    }

    fn disconnect(&mut self) {
        self.ven.write(Low);
    }

    fn reset(&mut self) {
        self.ven.write(Low);
        thread::sleep(Duration::from_millis(10));
        self.ven.write(High);
        thread::sleep(Duration::from_millis(100));
    }

    fn receive(&mut self, buffer: &mut [u8], timeout:std::time::Duration) -> Result<usize, TmlError> {
        let t = Instant::now();
        while (t.elapsed() < timeout) && (self.irq.read() == Low) {
            thread::sleep(Duration::from_millis(1));
        }
        //println!("RD t={:?}", t.elapsed());
        if self.irq.read() == Low {
            return Err(TmlError::Timeout);
        }

        let res = self.i2c.read(&mut buffer[0..3]);
        if res.is_ok() {
            let cnt = res.unwrap();
            let extra = buffer[2] as usize;
            if (cnt == 3) && (extra > 0) {
                let res = self.i2c.read(&mut buffer[3..(3+extra)]);
                if res.is_ok() {
                    return Ok(res.unwrap() + 3);
                } else {
                    return Err(TmlError::Read);
                }
            } else {
                return Err(TmlError::Read);
            }
        } else {
            return Err(TmlError::Read);
        }
    }    

    fn send(&mut self, buffer: &[u8]) -> Result<usize, TmlError> {
        let res = self.i2c.write(buffer);
        if res.is_err() {
            return Err(TmlError::Write);
        } else {
            return Ok(res.unwrap());
        }
    }
}

fn main() -> Result<(), Box<dyn Error>> {
        let mut tml:Tml = Tml::connect()?;
        tml.reset();

        let blocks = vec![
            vec![0x20u8, 0x00u8, 0x01u8, 0x00u8],
            vec![0x20u8, 0x01u8, 0x00u8],
            vec![0x2Fu8, 0x02u8, 0x00u8],
            vec![0x21u8, 0x03u8, 0x09u8, 0x04u8, 0x00u8, 0x01u8, 0x01u8, 0x01u8, 0x02u8, 0x01u8, 0x06u8, 0x01u8]
        ];

        for block in blocks {
            println!("WR: {:02X?}", &block[..]);
            tml.send(&block[..])?;

            let mut answ = [0xFFu8; 64];
            let res = tml.receive(&mut answ, std::time::Duration::from_millis(100))?;
            println!("RD: {:02X?}", &answ[0..res]);
            println!("");
        }
        return Ok(());
}
