// Copyright 2018 the Deno authors. All rights reserved. MIT license.
import * as fbs from "gen/msg_generated";
import { flatbuffers } from "flatbuffers";
import * as dispatch from "./dispatch";

/**
 * Synchronously creates newname as a symbolic link to oldname.
 * The type argument can be set to 'dir' or 'file' and is only
 * available on Windows (ignored on other platforms).
 *
 *     import { symlinkSync } from "deno";
 *     symlinkSync("old/name", "new/name");
 */
export function symlinkSync(
  oldname: string,
  newname: string,
  type?: string
): void {
  dispatch.sendSync(...req(oldname, newname, type));
}

/**
 * Creates newname as a symbolic link to oldname.
 * The type argument can be set to 'dir' or 'file' and is only
 * available on Windows (ignored on other platforms).
 *
 *     import { symlink } from "deno";
 *     await symlink("old/name", "new/name");
 */
export async function symlink(
  oldname: string,
  newname: string,
  type?: string
): Promise<void> {
  await dispatch.sendAsync(...req(oldname, newname, type));
}

function req(
  oldname: string,
  newname: string,
  type?: string
): [flatbuffers.Builder, fbs.Any, flatbuffers.Offset] {
  let type_;
  const builder = new flatbuffers.Builder();
  const oldname_ = builder.createString(oldname);
  const newname_ = builder.createString(newname);
  if (type) {
    type_ = builder.createString(type);
  }
  fbs.Symlink.startSymlink(builder);
  fbs.Symlink.addOldname(builder, oldname_);
  fbs.Symlink.addNewname(builder, newname_);
  if (type) {
    fbs.Symlink.addLinktype(builder, type_ as number);
  }
  const msg = fbs.Symlink.endSymlink(builder);
  return [builder, fbs.Any.Symlink, msg];
}
