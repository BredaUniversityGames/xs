---
layout: default
title: Assert
parent: API Reference
nav_order: 8
---

<!-- file: assets/modules/external/assert.wren -->
<!-- documentation automatically generated using domepunk/tools/doc -->
---
## [Class Assert](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L20)


* Minimalist assertion library for unit testing in Wren.
*
* @author Rob Loach (@RobLoach)
* @license MIT
* @website https://github.com/RobLoach/wren-assert
*
* Copyright 2020 Rob Loach (@RobLoach)
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
* documentation files (the "Software"), to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of
* the Software.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
* THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## API

### [static equal(actual, expected)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L24)


* Assert that the two given values are equal.


### [static equal(actual, expected, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L31)


* Assert that the two given values are equal, with a given message.


### [static [actual, expected]](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L42)


* Asserts that the two given values are equal.
*
* Alias for Assert.equal(actual, expected).


### [static [actual, expected, message]](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L51)


* Asserts that the two given values are equal, with a message.
*
* Alias for Assert.equal(actual, expected, message).


### [static notEqual(actual, expected)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L58)


* Assert that the two given values are not equal.


### [static notEqual(actual, expected, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L65)


* Assert that the two given values are not equal, with a given message.


### [static ok(value)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L74)


* Assert that the given value is truthy.


### [static ok(value, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L81)


* Assert that the given value is truthy, with a given message.


### [static notOk(value)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L92)


* Assert that the given value is falsey.


### [static notOk(value, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L99)


* Assert that the given value is falsey, with a given message.


### [static [value]](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L110)


* Asserts that the given value is ok.
*
* Alias for Assert.ok(value).


### [static aborts(fn)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L117)


* Assert that the given function aborts.


### [static aborts(fn, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L124)


* Assert that the given function aborts, with a given message.


### [static doesNotAbort(fn)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L135)


* Assert that the given function does not abort.


### [static doesNotAbort(fn, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L146)


* Assert that the given function does not abort, with a given message.


### [static typeOf(object, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L157)


* Assert that the given object matches the given type.


### [static typeOf(object, type, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L164)


* Assert that the given object matches the given type, with a message.


### [static notTypeOf(object, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L173)


* Assert that the given object doesn't match the given type.


### [static notTypeOf(object, type, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L180)


* Assert that the given object doesn't match the given type, with a message.


### [static countOf(list, count)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L189)


* Assert that a list matches a given count.


### [static countOf(list, count, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L196)


* Assert that a list matches a given count, with a message.


### [static deepEqual(actual, expected, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L203)


* Asserts that the two given objects, and their children, are equal, with a message.


### [static deepEqual(actual, expected)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L221)


* Asserts that the two given objects, and their children, are equal.


### [static exists(value, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L228)


* Asserts that the given value is not null.


### [static exists(value)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L235)


* Asserts that the given value is not null.


### [static notExists(value, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L242)


* Asserts that the given value is null.


### [static notExists(value)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L249)


* Asserts that the given value is null.


### [static contains(haystack, needle, message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L256)


* Asserts that the given sequence haystack contains the given needle value.


### [static fail(message)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L271)


* Throws an abort on the current fiber with the given message.


### [static fail()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L280)


* Throws an abort on the current fiber.


### [static fail(actual, expected, operator)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L287)


* Throws an abort on the current fiber, using a built message.


### [static fail(actual, expected)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L294)


* Throws an assert on the given fiber, assuming an equal operator.


### [static disabled](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L301)


* Gets whether or not assertions should be skipped.


### [static disabled=(value)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/external/assert.wren#L306)


* Set `Assert.disabled = true` to have assertions skip avoid throwing Fiber.aborts().

