// Copyright 2018 the Deno authors. All rights reserved. MIT license.

use hyper::{Uri, Error};

// pub fn uri_join(
//   base: Uri,
//   path: &str,
// ) -> Result<Uri, Error> {
//   // let base_parts = base.into_parts();
//   // let base_path_and_query = base_parts.path_and_query.unwrap();
//   // let base_path = base_path_and_query.path();
//   // let base_path_vec = base_path.split("/").collect::<Vec<&str>>();
//   // let path_vec = path.split("/").collect::<Vec<&str>>();
//   // let mut idx = 0;
//   // let len = path_vec.len();
//   // for i in 0..len {
//   //   if path_vec[i] == "." {
//   //     continue;
//   //   } else if path_vec[i] == ".." && base_path_vec.len() > 0 {
//   //     base_path_vec.pop();
//   //   } else {
//   //     base_path_vec.push(path_vec[i]);
//   //   }
//   // }
//   // let joined_path = base_path_vec.join("/");
//   // // let joined_path = String::from(base_path) + "/" + path;\
//   // let joined_path_and_query =
//   //   <Uri as Uri>::from_shared(joined_path.into()).unwrap();
//   // let mut joined_parts = <Uri as Uri>::Parts::default();
//   // joined_parts.scheme = base_parts.scheme;
//   // joined_parts.authority = base_parts.authority;
//   // joined_parts.path_and_query = Some(joined_path_and_query);

//   // <Uri as Uri>::from_parts(joined_parts)
// }

fn uri_join(
  base: Uri,
  path: &str,
) -> Result<Uri, Error> {
  let base_parts = base.into_parts();
  let base_path_and_query = base_parts.path_and_query.unwrap();
  let base_path = base_path_and_query.path();
  let joined_path = String::from(base_path) + "/" + path;
  let joined_path_and_query =
    Uri::PathAndQuery::from_shared(joined_path.into()).unwrap();
  let mut joined_parts = Uri::Parts::default();
  joined_parts.scheme = base_parts.scheme;
  joined_parts.authority = base_parts.authority;
  joined_parts.path_and_query = Some(joined_path_and_query);

  Uri::from_parts(joined_parts)
}

#[cfg(test)]
pub fn test_uri_join() {
  let base_uri = "/foo/bar?baz".parse::<Uri>().unwrap();
  let mut path1 = "./one/two/three";
  let path2 = uri_join(base_uri, path1);
  assert_eq!(path1.into_parts().parts.path_and_query.unwrap(), "/foo/bar/one/two/three?bar");
  let mut path3 = "../one/two/three";
  let path4 = uri_join(base_uri, path3);
  assert_eq!(path4.into_parts().parts.path_and_query.unwrap(), "/fo/one/two/three?bar");
}