
import * as React from 'react'
import { createStackNavigator } from '@react-navigation/stack'
import {NavigationContainer} from '@react-navigation/native';
import HomeScreen from '../screens/home';
import HomeSetupScreen from '../screens/homeSetup';

const Stack = createStackNavigator();
function MainStackNavigator() {
    return (
        <NavigationContainer>
            <Stack.Navigator initialRouteName='HomeScreen'>
                <Stack.Screen name='HomeScreen' component={HomeScreen} options={{ title: 'Venti - Home' }} />
                <Stack.Screen name='HomeSetupScreen' component={HomeSetupScreen} options={{ title: 'Venti - Home Setup' }} />
            </Stack.Navigator>
        </NavigationContainer>
    )
}

export default MainStackNavigator;
