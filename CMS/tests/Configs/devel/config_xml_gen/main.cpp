#include <stdio.h>
#include <string>
#include <regex>
#include <set>
#include <map>
struct st_malloc
{
private:
    st_malloc(const st_malloc&);             // Not defined
    st_malloc& operator=(const st_malloc&);  // Not defined
public:
    unsigned char *buf;
    st_malloc(size_t size)
    {
        buf = (unsigned char*)malloc(size + 1);
        //if (buf == NULL)throw std::runtime_error("malloc error");
    }
    ~st_malloc()
    {
        if (buf)
        {
            free(buf);
            buf = NULL;
        }
    }

};
std::string loadFile(const std::string& name)
{
    FILE *f = fopen(name.c_str(), "rb");
    if (f==nullptr)
    {
        printf("fopen error: %s\n",name.c_str());
        return "";
    }
    auto res=fseek(f,0,SEEK_END);
    if(res!=0)
    {
        fclose(f);
        printf("fseek error %s\n",name.c_str());
        return "";
    }
    long fsize=ftell(f);
    if(fsize==-1)
    {
        fclose(f);
        printf("ftell error %s\n",name.c_str());
        return "";

    }
    auto res2=fseek(f,0,SEEK_SET);
    if(res2!=0)
    {
        fclose(f);
        printf("fseek error 2 %s\n",name.c_str());
        return "";
    }

    st_malloc buf(fsize);
    auto fres=fread(buf.buf,fsize,1,f);
    if(fres!=1)
    {
        fclose(f);
        printf("fread error %s\n",name.c_str());
        return "";

    }
    fclose(f);
    return  std::string((char*)buf.buf,fsize);

}
std::vector<std::string> splitString(const char *seps, const std::string & src)
{


    std::vector < std::string> res;
    std::set<char>mm;
    size_t l;
    l =::strlen(seps);
    for (unsigned int i = 0; i < l; i++)
    {
        mm.insert(seps[i]);
    }
    std::string tmp;
    l = src.size();

    for (unsigned int i = 0; i < l; i++)
    {

        if (!mm.count(src[i]))
            tmp+=src[i];
        else
        {
            if (tmp.size())
            {
                res.push_back(tmp);
                tmp.clear();
            }
        }
    }

    if (tmp.size())
    {
        res.push_back(tmp);
        tmp.clear();
    }
    return res;
}
std::string escape(const std::string& s)
{
    std::set<char> special_chars={ '.' , '+' , '*' , '?' , '^' , '$' , '(' , ')' , '[' , ']' , '{' , '}' , '|' , '\\'};
    std::string out;
    for(auto &c:s)
    {
        if(special_chars.count(c))
            out+="\\";
        out+=c;
    }
    return out;
}
std::string join(const char* pattern, const std::vector<std::string>& arr)
{

    std::string ret;
    if (arr.size() > 1)
    {
        unsigned int i;
        for (i = 0; i < arr.size() - 1; i++)
            ret += arr[i] + pattern;
        ret += arr[arr.size() - 1];
    }
    else if (arr.size() == 1)
    {
        ret += arr[0];
    }
    return ret;

}
std::string replaceN()
{
    return "";
}
int main(int argc, char *argv[])
{
    auto body=loadFile("colocation.xml.t");
    if(body.size()==0)
        throw std::runtime_error("failed open file colocation.xml.t");
    auto devvars_b=loadFile("devvars.t");
    if(devvars_b.size()==0)
        throw std::runtime_error("failed open file devvars.t");
    auto dv=splitString("\r\n",devvars_b);
    std::map<std::string,std::string> replacements;
    std::map<std::string,std::string> replacements_all;
    for(auto &line: dv)
    {
        auto parts=splitString("=",line);
        if(parts.size()!=2)
            throw std::runtime_error("wrong parts in line " + line);
        if(parts[0].size()&& parts[0][0]=='$')
            replacements[parts[0]]=parts[1];
        replacements_all[parts[0]]=parts[1];
    }
//    std::string regex_string;
    std::vector<std::string> re_v;
    for(auto &r:replacements)
    {
        re_v.push_back(escape(r.first));
    }
    auto re_s=join("|",re_v);
//    re_s+="|\\[%[\\s]+\\+[\\s]+PORT_BASE[\\s]+\\+[\\s]+([\\d+])[\\s]+%\\]";
//    re_s+="|\\[%[\\s\\d\\w]+%\\]";
    re_s+="|[%[\\s\\d\\w\\+]+%\\]";
    printf("re_s %s\n",re_s.c_str());

    std::regex re(re_s);

    const std::vector<std::smatch> matches{
        std::sregex_iterator{body.cbegin(), body.cend(), re}
        ,
        std::sregex_iterator{}
    };

//    std::string out=body;
    std::vector<std::pair<std::pair<int/*pos*/, int/*len*/>,std::string> > vrep;
    for(auto& m: matches)
    {
        printf("m %s\n",m.str().c_str());

        if(replacements.count(m.str()))
            vrep.push_back({{m.position(),m.length()},replacements[m.str()]});
        else if(m.str().substr(0,2)=="[%" && m.str().substr(m.str().size()-2,2)=="%]")
        {
            std::string port=m.str().substr(2,m.str().size()-4);
            printf("OOOOOOO!  %s\n",port.c_str());
            auto ps=splitString(" +",port);
            if(ps.size()!=2)
                throw std::runtime_error("invalid port "+port);
            if(!replacements_all.count(ps[0]))
                throw std::runtime_error("invalid name "+ps[0]);

            int port_i=atoi(replacements_all[ps[0]].c_str())+atoi(ps[1].c_str());
            printf("port_i %d\n",port_i);
            vrep.push_back({{m.position(),m.length()},std::to_string(port_i)});
        }


//        printf("m.size() %d\n",m.size());
//        for(int i=0;i<m.size();i++)
//        {

//            printf ("m[i].str() %s\n ",m[i].str().c_str());
//        }
//        auto it=replacements.find(m.str());
//        if(it!=
    }
    std::string out;
    for(size_t i=0;i<vrep.size();i++)
    {
        if(i==0)
        {
            out+=body.substr(0,vrep[i].first.first);
        }
        else
        {
            int S=vrep[i-1].first.first+vrep[i-1].first.second;
            int E=vrep[i].first.first;
            int L=E-S;
            out+=body.substr(S,L);
        }
        out+=vrep[i].second;
        if(i==vrep.size()-1)
        {
            int S=vrep[i].first.first+vrep[i].first.second;
            out+=body.substr(S,body.size()-S);
        }
    }
    printf("%s\n",out.c_str());
    return 0;
}
