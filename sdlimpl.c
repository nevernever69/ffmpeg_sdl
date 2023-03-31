/* #include<main.c> */
#include<SDL2/SDL.h>
#include<SDL2/SDL_thread.h>

int main(){
	SDL_Window *window = NULL;
	SDL_Surface *windowsurface = NULL;
	SDL_Surface *imagesurface = NULL;
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		printf("error1");
	
	}else{

		window = SDL_CreateWindow("SDL Coding",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640,480,SDL_WINDOW_SHOWN);
		if(window==NULL){
			printf("error2");
		}else{
			windowsurface = SDL_GetWindowSurface(window);
			imagesurface = SDL_LoadBMP("new.ppm");
			if(imagesurface == NULL){
				printf("error3");
			}
			else{
				SDL_BlitSurface(imagesurface,NULL,windowsurface,NULL);
			SDL_UpdateWindowSurface(window);
			SDL_Delay(20000);
			}
				
		}

	}
	SDL_FreeSurface(imagesurface);
	SDL_DestroyWindow(window);

	SDL_Quit();




}
