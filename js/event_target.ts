// Copyright 2018 the Deno authors. All rights reserved. MIT license.
import { EventTarget, Event } from "./dom_types";
import { type } from "os";

export class EventTargetImpl implements EventTarget {
  private listeners: Map<string, EventListenerOrEventListenerObject[]>;

  constructor() {
    this.listeners = new Map();
  }

  addEventListener(
    type: string,
    listener: EventListenerOrEventListenerObject | null,
    options: EventListenerOptions | boolean
  ): void {
    if (listener === null) {
      return;
    }

    if (!this.listeners.has(type)) {
      let list = this.listeners.get(type);
      if (list) {
        list.push(listener);
      } else {
        list = [];
      }
      this.listeners.set(type, list);
    }
  }

  dispatchEvent(event: Event): boolean {
    return true;
  }

  removeEventListener(
    type: string,
    listener?: EventListenerOrEventListenerObject | null,
    options?: EventListenerOptions | boolean
  ): void {
    if (listener === null) {
      return;
    }

    if (!this.listeners.has(type)) {
      return;
    }

    const stack = this.listeners.get(type);
    for (let i = 0; i < stack.length; i++) {
      if (stack[i] === listener) {
        stack.splice(i, 1);
        return this.removeEventListener(type, callback);
      }
    }
  }
}
