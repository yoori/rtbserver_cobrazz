
class MinioRequest:
    def __init__(self, client, bucket, name):
        self.__response = client.get_object(bucket, name)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.__response.close()
        self.__response.release_conn()
        self.__response = None

    def stream(self):
        return self.__response.stream()

    @property
    def data(self):
        return self.__response.data

