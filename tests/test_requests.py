
import tests.common
import requests
from tests.common import eq,contains,stop_server

#process.terminate()

server = None
def setup():
  print("Begin test_requests")
  global server
  server = tests.common.start_server("tests/s1.py")  
  if not server:
    return 1
  return 0


def test_one():
  r = requests.get('http://localhost:8080/foo')
  eq(r.status_code, 200)
  r = requests.get('http://localhost:8080/to64')
  eq(r.text, "ok")
  r = requests.get('http://localhost:8080/foo/baz/whatever')
  eq(r.text, "foo2")
  r = requests.get('http://localhost:8080/foo/bar/whatever')
  eq(r.text, "foo3")
  r = requests.get('http://localhost:8080/foo/baz/w')
  eq(r.text, "foo2")
  r = requests.get('http://localhost:8080/foo/bar/w')
  eq(r.text, "foo3")
  r = requests.get('http://localhost:8080/food/bar/w')
  eq(r.text, "barmid")
  r = requests.get('http://localhost:8080/food/bar/whhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh/')
  eq(r.text, "barmid")
  r = requests.get('http://localhost:8080/print/caf%C3%A9/')
  eq(r.text, "café")
  r = requests.get('http://localhost:8080/print/%E4%B8%8D%E5%8F%AF%E5%86%8D%E7%94%9F%E8%B5%84%E6%BA%90/?test')
  eq(r.text, "不可再生资源")

  # Multiple args
  r = requests.get('http://localhost:8080/args/a/b/c/d/e/f/')
  eq(r.text, "abcdef")
  r = requests.get('http://localhost:8080/args/5/')
  eq(r.text, "5")
  r = requests.get('http://localhost:8080/args/9999999999/')
  eq(r.text, "9999999999")

  # Query string ?foo=bar
  r = requests.get('http://localhost:8080/query_string')
  eq(r.text, "{}")
  r = requests.get('http://localhost:8080/query_string?foo=bar')
  eq(r.text, "{'foo': 'bar'}")
  r = requests.get('http://localhost:8080/query_string?a=b&foo=bar&ABCDE=01234567890123456789')
  eq(r.text, "{'a': 'b', 'foo': 'bar', 'ABCDE': '01234567890123456789'}")

  # requests orders cookie keys alphabetically looks like
  r = requests.post('http://localhost:8080/printPostBody', json={"key": "value"})
  eq(r.text, '{"key": "value"}')
  cookie = {'foo': 'bar'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'foo': 'bar'}")
  cookie = {'foo': 'bar','yay':'test'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'foo': 'bar', 'yay': 'test'}")
  cookie = {'foo': 'bar','baz':'3'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'baz': '3', 'foo': 'bar'}")

  # Form handling
  r = requests.post('http://localhost:8080/form', data={"p1":"v1","param2":"value2"})
  eq(r.text, "value2")
  r = requests.get('http://localhost:8080/form')
  eq(r.text, "No form")

  # Sessions
  cookie = {'mrsession': '43709dd361cc443e976b05714581a7fb'}
  r = requests.post('http://localhost:8080/s', cookies=cookie)
  eq(r.text, "session")


  #contains(r.text, "Internal Server Error")
  #print( r.status_code, r.headers, r.text )

def teardown():
  global server
  stop_server(server)
  #server.terminate()

