import pygame
import sys
import math
import random

SCREEN_WIDTH = 1200

pygame.init()
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_WIDTH))
pygame.display.set_caption("Ice Ice Baby")

class Lattice2D:
	def __init__(self, n, B):
		self.n = n
		self.B = B
		self.sites = [random.choices((-1,1), k=n) for _ in range(n)]
	
	def update(self):
		j = random.randrange(self.n**2)
		i = j%self.n
		j //= self.n
		num_matching = [self.sites[i2%self.n][j2%self.n] for (i2,j2) in ((i+1,j),(i-1,j),(i,j+1),(i,j-1))].count(self.sites[i][j])
		# energy before flipping is 4 - 2*num_matching
		# energy after flipping is 4 - 2*(4 - num_matching) = 2*num_matching - 4
		# so energy decreases if 2*num_matching - 4 <= 4 - 2*num_matching <-> 4*num_matching <= 8 <-> num_matching <= 2
		# and otherwise the change in energy is 4*num_matching - 8
		
		if num_matching <= 2 or random.random() <= math.exp(self.B*(8 - 4*num_matching)):
			self.sites[i][j] *= -1
	
	def draw(self, screen):
		screen.fill(0xFFFFFFFF)
		S = SCREEN_WIDTH/self.n
		for i, row in enumerate(self.sites):
			for j, spin in enumerate(row):
				if spin == -1:
					pygame.draw.rect(screen, 0xFF000000, pygame.Rect(j*S, i*S, S, S))

clock = pygame.time.Clock()
critical_B = 0.44068679350977147
latt = Lattice2D(200, 0.44068679350977147)

screen.fill(0xFFFFFFFF)
latt.draw(screen)
while True:
	for e in pygame.event.get():
		if e.type == pygame.QUIT or (e.type == pygame.KEYDOWN and e.key == pygame.K_ESCAPE):
			pygame.quit()
			sys.exit(0)
		if e.type == pygame.KEYDOWN and e.key == pygame.K_RIGHT:
			pass
	for _ in range(1000):
		latt.update()
	latt.draw(screen)
	pygame.display.flip()

