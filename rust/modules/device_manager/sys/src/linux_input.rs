/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! TODO: add documentation.

#![allow(dead_code)]

const _IOC_READ: usize = 2usize;
const _IOC_NRBITS: usize = 8usize;
const _IOC_TYPEBITS: usize = 8usize;
const _IOC_SIZEBITS: usize = 14usize;
const _IOC_DIRBITS: usize = 2usize;
const _IOC_NRSHIFT: usize = 0usize;
const _IOC_TYPESHIFT: usize = _IOC_NRSHIFT + _IOC_NRBITS;
const _IOC_SIZESHIFT: usize = _IOC_TYPESHIFT + _IOC_TYPEBITS;
const _IOC_DIRSHIFT: usize = _IOC_SIZESHIFT + _IOC_SIZEBITS;

const fn _ioc(dir: usize, _type: usize, nr: usize, size: usize) -> usize
{
    (dir  << _IOC_DIRSHIFT) |
	(_type << _IOC_TYPESHIFT) |
	(nr   << _IOC_NRSHIFT) |
	(size << _IOC_SIZESHIFT)
}

const fn _ior(a: usize, b: usize, c: usize) -> usize
{
    _ioc(_IOC_READ,a,b,c)
}

/// get device name
pub const fn eviocgname(len: usize) -> usize
{
    _ioc(_IOC_READ, 'E' as usize, 0x06, len)
}

/// get physical location
pub const fn eviocgphys(len: usize) -> usize
{
    _ioc(_IOC_READ, 'E' as usize, 0x07, len)
}

/// get unique identifier
pub const fn eviocguniq(len: usize) -> usize
{
    _ioc(_IOC_READ, 'E' as usize, 0x08, len)
}

/// get device properties
pub const fn eviocgprop(len: usize) -> usize
{
    _ioc(_IOC_READ, 'E' as usize, 0x09, len)
}

/// get event bits
pub const fn eviocgbit(ev:usize,len:usize) -> usize
{
    _ioc(_IOC_READ, 'E' as usize, 0x20 + (ev), len)
}

/// get device ID
pub fn eviocgid() -> usize
{
    _ior('E' as usize, 0x02, std::mem::size_of::<libc::input_id>())
}
