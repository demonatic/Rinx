# libRinx

###### USTC  软院  2019工程实践 子项目

libRinx is a server-side HTTP library aims at equipping your program with HTTP service easily. It internally uses reactor model and non-blocking IO to handle concurrency.

## Implementation
| 0x01                        | 0x02                                   | 0x03                                      | 0x04                                    |
| --------------------------- | -------------------------------------- | ----------------------------------------- | --------------------------------------- |
| [项目概览](./include/README.md) | [索引实现](./src/core/index/README.md) | [Schema实现](./src/core/schema/README.md) | [查询实现](./src/core/search/README.md) |



## Build

## Usage Example

```c++
int main(){
      RxServer server;
      RxProtocolHttp1Factory http1;
    
      // hello world example    
      http1.GET("/hello",[](HttpRequest &req,HttpResponse &resp){
        resp.status_code(HttpStatusCode::OK).headers("content-type","text/plain");
        resp.body()<<"hello world";
      });
      server.listen("0.0.0.0",8080,http1);
}
```
We can use regex expression as route. Specifying a content_generator is also helpful when you want to generate and send a large content on the fly.
```c++
  // send some large content K times using content_generator
    std::string large_content="...";
    http1.GET(R"(/large/(\d+))",[&](HttpRequest &req,HttpResponse &resp){
        int times=stoi(req.matches[1]);
        //use chunk-encoding automatically if doesn't specify content-length in header
        resp.status_code(HttpStatusCode::OK);
        resp.content_generator([&,times](HttpResponse::ProvideDone done) mutable{
             if(times--){
                 resp.body()<<large_content;
             }
             else{
                 done(); //call done() when generation is over
             }
        });
    });
```

The handler by default runs in IO threads. If you want to do costly task, just wrapping the handler in "MakeAsync", which would execute your handler in an internal thread pool. 
```c++
 
    // solve sudoku in internal thread pool asynchronously
    http1.POST("/sudoku",MakeAsync([](HttpRequest &req,HttpResponse &resp){
		// note:this function is called from a thread in thread pool
        vector<vector<char>> board(9,vector<char>(9,'\0'));
        int count=0;
        //request body provides an iterator
        for(auto it=req.body().begin();it!=req.body().end();++it){
            board[count/9][count%9]=*it;
            count++;
        }
	
        SudokuSolver sudoku_solver;
        sudoku_solver.solve(board);

        std::string solve_res;
        for(auto &v:board){
            for(char c:v){
                solve_res.push_back(c);
            }
            cout<<endl;
        }
        resp.status_code(HttpStatusCode::OK).body()<<solve_res;
    }));

```

Set default handler when no callback matches.
```c++
    http1.default_handler([](HttpRequest &req,HttpResponse &resp){
        resp.send_status(HttpStatusCode::FORBIDDEN);
        req.close_connection();
    });
```



