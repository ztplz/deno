// Copyright 2018 the Deno authors. All rights reserved. MIT license.

import * as deno from "deno";
import { test, testPerm, assert, assertEqual } from "./test_util.ts";

test(function filesStdioFileDescriptors() {
  assertEqual(deno.stdin.fd, 0);
  assertEqual(deno.stdout.fd, 1);
  assertEqual(deno.stderr.fd, 2);
});

test(async function filesCopyToStdout() {
  const filename = "package.json";
  const file = await deno.open(filename);
  assert(file.fd > 2);
  const bytesWritten = await deno.copy(deno.stdout, file);
  const fileSize = deno.statSync(filename).len;
  assertEqual(bytesWritten, fileSize);
  console.log("bytes written", bytesWritten);
});

testPerm({ write: true }, async function createFileSuccess() {
  const path = deno.makeTempDirSync() + "/dir.txt";
  const f = await deno.create(path);
  assert(f instanceof deno.File);
  const fileInfo = deno.statSync(path);
  assert(fileInfo.len === 0);
  assert(fileInfo.isFile());
});
