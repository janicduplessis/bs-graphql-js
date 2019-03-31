open Schema;

[@bs.module "graphql-relay"]
external fromGlobalId:
  string =>
  {
    .
    "id": string,
    "type": string,
  } =
  "";

[@bs.module "graphql-relay"]
external toGlobalId: (~typ: string, ~localId: string) => string = "";

[@bs.module "graphql-relay"]
external connectionFromArray_:
  (
    array('a),
    {
      .
      "first": Js.nullable(int),
      "last": Js.nullable(int),
      "before": Js.nullable(string),
      "after": Js.nullable(string),
    }
  ) =>
  'b =
  "connectionFromArray";

/* Not sure if there is a way to avoid this... */
module type AppContext = {type t;};

module Node = {
  [@bs.module "graphql-relay"] [@bs.val]
  external toGlobalId: (~typ: string, ~id: string) => string = "";
  [@bs.module "graphql-relay"] [@bs.val]
  external fromGlobalId:
    string =>
    {
      .
      "type": string,
      "id": string,
    } =
    "";
  let globalIdField = (typeName: string, ~resolve) =>
    field(
      "id",
      ~typ=nonNull(id),
      ~args=[],
      ~doc="The ID of an object",
      ~resolve=(_ctx, node) =>
      toGlobalId(~typ=typeName, ~id=resolve(node))
    );
};

module Connection = {
  type pageInfo = {
    hasNextPage: bool,
    hasPreviousPage: bool,
    startCursor: option(string),
    endCursor: option(string),
  };
  type edge('t) = {
    node: 't,
    cursor: string,
  };
  type t('t) = {
    edges: array(edge('t)),
    pageInfo,
  };
  let empty = () => {
    edges: [||],
    pageInfo: {
      hasNextPage: false,
      hasPreviousPage: false,
      startCursor: None,
      endCursor: None,
    },
  };
  let fromArray =
      (
        ~first: option(int)=?,
        ~last: option(int)=?,
        ~before: option(string)=?,
        ~after: option(string)=?,
        array: array('a),
      )
      : t('a) => {
    let jsConnection =
      connectionFromArray_(
        array,
        {
          "first": first |> Js.Nullable.fromOption,
          "last": last |> Js.Nullable.fromOption,
          "before": before |> Js.Nullable.fromOption,
          "after": after |> Js.Nullable.fromOption,
        },
      );
    {
      edges:
        jsConnection##edges
        |> Array.map(edge => {node: edge##node, cursor: edge##cursor}),
      pageInfo: {
        hasNextPage: jsConnection##pageInfo##hasNextPage,
        hasPreviousPage: jsConnection##pageInfo##hasPreviousPage,
        startCursor: jsConnection##pageInfo##startCursor |> Js.toOption,
        endCursor: jsConnection##pageInfo##endCursor |> Js.toOption,
      },
    };
  };
  let forwardsArgs:
    type a. unit => Arg.argList(a, (option(int), option(string)) => a) =
    () => {
      open! Arg;
      [arg("first", ~typ=int), arg("after", ~typ=string)];
    };
  let pageInfoType = (): typ('a, option(pageInfo)) =>
    /* Needs a different name than the one in graphql-relay */
    obj(
      "BsPageInfo",
      ~doc="Information about pagination in a connection.",
      ~fields=
        lazy [
          field(
            "hasNextPage",
            ~typ=nonNull(bool),
            ~args=[],
            ~doc="When paginating forwards, are there more items?",
            ~resolve=(_ctx, node) =>
            node.hasNextPage
          ),
          field(
            "hasPreviousPage",
            ~typ=nonNull(bool),
            ~args=[],
            ~doc="When paginating backwards, are there more items?",
            ~resolve=(_ctx, node) =>
            node.hasPreviousPage
          ),
          field(
            "startCursor",
            ~typ=string,
            ~args=[],
            ~doc="When paginating backwards, the cursor to continue.",
            ~resolve=(_ctx, node) =>
            node.startCursor
          ),
          field(
            "endCursor",
            ~typ=string,
            ~args=[],
            ~doc="When paginating forwards, the cursor to continue.",
            ~resolve=(_ctx, node) =>
            node.endCursor
          ),
        ],
    );
  type definition('a, 'b) = {
    edgeType: 'a,
    connectionType: 'b,
  };
  let definitions = (name, ~nodeType) => {
    let edgeType =
      obj(
        name ++ "Edge",
        ~doc="An edge in a connection.",
        ~fields=
          lazy [
            field(
              "node",
              ~typ=nonNull(nodeType),
              ~args=[],
              ~resolve=(_ctx, node) => node.node,
              ~doc="The item at the end of the edge",
            ),
            field(
              "cursor",
              ~typ=nonNull(string),
              ~args=[],
              ~resolve=(_ctx, node) => node.cursor,
              ~doc="A cursor for use in pagination",
            ),
          ],
      );
    let connectionType =
      obj(
        name ++ "Connection",
        ~doc="A connection to a list of items.",
        ~fields=
          lazy [
            field(
              "pageInfo",
              ~typ=nonNull(pageInfoType()),
              ~args=[],
              ~doc="Information to aid in pagination.",
              ~resolve=(_ctx, node) =>
              node.pageInfo
            ),
            field(
              "edges",
              ~typ=nonNull(list(nonNull(edgeType))),
              ~args=[],
              ~doc="A list of edges.",
              ~resolve=(_ctx, node) =>
              node.edges
            ),
          ],
      );
    {edgeType, connectionType};
  };
};
