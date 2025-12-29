-- ghc -O2 -threaded -o birthday Birthday.hs && ./birthday +RTS -N
{-# LANGUAGE BangPatterns #-}

import Control.Concurrent (forkIO, newEmptyMVar, putMVar, takeMVar)
import Control.Monad (foldM, forM, replicateM)
import Control.Monad.ST (ST, runST)
import Data.Array.Base (unsafeRead, unsafeWrite)
import Data.Array.ST (STUArray, newArray)
import Data.Bits (xor)
import Data.Time.Clock.POSIX (getPOSIXTime)
import Data.Word (Word32, Word64, Word8)
import Text.Printf (printf)

daysInYear, numThreads, people, totalSimulations, simulationsPerThread :: Int
daysInYear = 365
numThreads = 768
people = 24
totalSimulations = 1_000_000
simulationsPerThread = totalSimulations `div` numThreads

multiplier, increment :: Word32
multiplier = 1664525
increment = 1013904223

simulate :: Int -> Int -> IO Int
simulate simulations threadId = do
  seedTime <- getPOSIXTime
  let seed = round (seedTime * 1e9) :: Word64
  let state = fromIntegral (seed `xor` fromIntegral threadId) :: Word32
  return $ runST $ do
    let loopSim !sim !state !localSuccessCount
          | sim == simulationsPerThread = return localSuccessCount
          | otherwise = do
              birthdays <- newArray (0, daysInYear - 1) 0 :: ST s (STUArray s Int Word8)
              let loopPeople !i !state
                    | i == people = return state
                    | otherwise = do
                        let state' = state * multiplier + increment
                        let birthday = fromIntegral (state' `rem` fromIntegral daysInYear)
                        day <- unsafeRead birthdays birthday
                        unsafeWrite birthdays birthday (day + 1)
                        loopPeople (i + 1) state'
              state <- loopPeople 0 state
              let loopCount !i !exactlyTwoCount
                    | i == daysInYear = return exactlyTwoCount
                    | otherwise = do
                        count <- unsafeRead birthdays i
                        loopCount (i + 1) (if count == 2 then exactlyTwoCount + 1 else exactlyTwoCount)
              exactlyTwoCount <- loopCount 0 0
              let localSuccessCount' = if exactlyTwoCount == 1 then localSuccessCount + 1 else localSuccessCount
              loopSim (sim + 1) state localSuccessCount'
    loopSim 0 state 0

main :: IO ()
main = do
  startTime <- getPOSIXTime
  threads <- replicateM numThreads newEmptyMVar
  successCount <- forM [0 .. numThreads - 1] $ \t -> do
      count <- newEmptyMVar
      forkIO $ putMVar count =<< simulate totalSimulations t
      return count
  totalSuccessCount <- sum <$> mapM takeMVar successCount
  let probability = fromIntegral totalSuccessCount / fromIntegral totalSimulations :: Double
  printf "Probability: %.9f\n" probability
  endTime <- getPOSIXTime
  let elapsedTime = realToFrac (endTime - startTime) :: Double
  printf "Execution Time: %.3f s\n" elapsedTime
