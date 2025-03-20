#!/usr/bin/env python3
"""
Linear Regression Plot Generator

This script generates plots for linear regression models using data passed from the C++ application.
It expects input data containing actual values, predicted values, and feature names.

Usage:
    Called from the C++ application with appropriate arguments.
    
Arguments:
    --data_file: Path to CSV file containing x and y values
    --model_file: Path to CSV file containing model predictions
    --output_file: Path to save the plot
    --x_column: Name of the X column
    --y_column: Name of the Y column
    --title: Plot title
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import os
import sys

def create_regression_plot(data_file, model_file, output_file, x_column, y_column, title):
    try:
        # Load data
        data = pd.read_csv(data_file)
        model_data = pd.read_csv(model_file)
        
        # Extract x and y values
        x_values = data[x_column].values
        y_values = data[y_column].values
        y_pred = model_data['predicted'].values
        
        # Create the plot
        plt.figure(figsize=(10, 6))
        
        # Plot actual data points
        plt.scatter(x_values, y_values, alpha=0.6, label='Actual Data')
        
        # Plot regression line
        plt.plot(x_values, y_pred, 'r-', linewidth=2, label='Model Prediction')
        
        # Add labels and title
        plt.title(title)
        plt.xlabel(x_column)
        plt.ylabel(y_column)
        plt.legend()
        plt.grid(True, linestyle='--', alpha=0.7)
        
        # Get directory for output file
        output_dir = os.path.dirname(output_file)
        if output_dir and not os.path.exists(output_dir):
            os.makedirs(output_dir)
        
        # Save the figure
        plt.savefig(output_file, format='png', dpi=150)
        print(f"Plot saved as: {os.path.abspath(output_file)}")
        
        return True
    except Exception as e:
        print(f"Error creating regression plot: {str(e)}", file=sys.stderr)
        return False

def main():
    parser = argparse.ArgumentParser(description='Generate linear regression plots')
    parser.add_argument('--data_file', required=True, help='Path to CSV file containing x and y values')
    parser.add_argument('--model_file', required=True, help='Path to CSV file containing model predictions')
    parser.add_argument('--output_file', required=True, help='Path to save the plot')
    parser.add_argument('--x_column', required=True, help='Name of the X column')
    parser.add_argument('--y_column', required=True, help='Name of the Y column')
    parser.add_argument('--title', default='Linear Regression Results', help='Plot title')
    
    args = parser.parse_args()
    success = create_regression_plot(
        args.data_file, 
        args.model_file, 
        args.output_file,
        args.x_column,
        args.y_column,
        args.title
    )
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()