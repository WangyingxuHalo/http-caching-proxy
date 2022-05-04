# ECE568-project2-HTTP_Caching_Proxy

Team Member:<br />
Yingxu Wang yw473<br />
Yiling Han yh317<br />

Project Intro:<br />
In this project, users can visit a website using the proxy we create.<br />
Types of request we accept: GET, POST and CONNECT<br />
If a website response is cacheable, we may store it in the cache for future use.<br />

Project Idea: (Procedure image was retrived from one TA)<br />
![](procedure.jpeg)<br />

In your browser proxy setting, set proxy:<br />
proxy address<br />
proxy ports(12345)<br />

Note:<br />
-Websites we have tested are inside the directory: ./docker_deploy/src/hw2/test_website.txt<br />
-Corresponding logs are inside the directory: ./docker_deploy/src/hw2/test_log.log<br />

**Enter these commands in order to start:**[**!important**]<br />
1.<br />
sudo chmod 777 -R . <br />
2. <br />
sudo docker-compose up
