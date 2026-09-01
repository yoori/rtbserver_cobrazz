from .McpClient import McpError


class McpClientPool:
  def __init__(self, clients):
    self.clients = tuple(clients)
    self.tools = []
    self.tool_clients = {}

  def start(self):
    if self.tool_clients:
      return
    tools = []
    tool_clients = {}
    try:
      for client in self.clients:
        for tool in client.list_tools():
          name = tool.get('name')
          if not name:
            raise McpError('MCP server returned a tool without a name')
          if name in tool_clients:
            raise McpError('duplicate MCP tool name: ' + name)
          tool_clients[name] = client
          tools.append(tool)
    except Exception:
      self.close()
      raise
    self.tools = tools
    self.tool_clients = tool_clients

  def list_tools(self):
    self.start()
    return list(self.tools)

  def call_tool(self, name, arguments):
    self.start()
    client = self.tool_clients.get(name)
    if client is None:
      raise McpError('unknown MCP tool: ' + name)
    return client.call_tool(name, arguments)

  def close(self):
    self.tools = []
    self.tool_clients = {}
    for client in self.clients:
      client.close()
